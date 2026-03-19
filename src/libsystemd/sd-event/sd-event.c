/*
 * Zephyr RTOS based implementation of systemd sd-event API
 *
 * Mapping to Zephyr primitives:
 * - Event loop -> k_poll + work queue
 * - Timers -> k_timer
 * - IO events -> zsock_poll (for sockets)
 * - Signals -> k_poll_signal
 * - Deferred/Post/Exit -> k_work
 */

#include <systemd/sd-event.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/dlist.h>

// Include dispatch context from dbus-broker
#include <util/dispatch.h>
#include <fcntl.h>

#ifdef __ZEPHYR__
#ifndef CONFIG_SD_EVENT_LOG_LEVEL
#define CONFIG_SD_EVENT_LOG_LEVEL LOG_LEVEL_INF
#endif
LOG_MODULE_REGISTER(sd_event, CONFIG_SD_EVENT_LOG_LEVEL);
#endif

/* Maximum number of event sources per event loop */
#ifndef CONFIG_SD_EVENT_MAX_SOURCES
#define CONFIG_SD_EVENT_MAX_SOURCES 32
#endif

/* Maximum number of IO sources for poll */
#ifndef CONFIG_SD_EVENT_MAX_IO_SOURCES
#define CONFIG_SD_EVENT_MAX_IO_SOURCES 16
#endif

/* Default accuracy for timers */
#ifndef DEFAULT_ACCURACY_USEC
#define DEFAULT_ACCURACY_USEC (250 * USEC_PER_MSEC)
#endif

#ifndef EPOLLIN
#define EPOLLIN 0x001
#endif

/* Default priority for event sources */
#define SD_EVENT_DEFAULT_PRIORITY 0

/* ============================================================
 * Internal Structures
 * ============================================================ */
struct sd_event_source {
    CList link;                      /* For source lists */

    enum sd_event_source_type type;
    enum sd_event_enabled enabled;

    sd_event *event;
    char *description;
    void *userdata;
    sd_event_destroy_t destroy_callback;

    int64_t priority;
    bool floating;
    bool pending;

    sd_event_handler_t prepare_callback;

    /* For IO sources - integrated with dispatch_context */
    DispatchFile dispatch_file;      /* Must be first for IO sources */
    
    /* Type-specific data */
    union {
        struct {
            int fd;
            uint32_t events;
            uint32_t revents;
            sd_event_io_handler_t handler;
        } io;

        struct {
            struct k_timer timer;
            int clock;
            uint64_t usec;
            uint64_t accuracy;
            sd_event_time_handler_t handler;
            int notify_pipe[2];      /* Notification pipe for timer */
            struct k_work_delayable work;  /* Delayed work for timer */
            bool expired;
        } time;

        struct {
            int sig;
            struct k_poll_signal signal;
            sd_event_signal_handler_t handler;
        } signal;

        struct {
            pid_t pid;
            int options;
            sd_event_child_handler_t handler;
        } child;

        struct {
            struct k_work work;
            sd_event_handler_t handler;
        } defer;

        struct {
            struct k_work work;
            sd_event_handler_t handler;
        } post;

        struct {
            struct k_work work;
            sd_event_handler_t handler;
        } exit;
    } data;

    atomic_t ref_count;
};

struct sd_event {
    DispatchContext dispatch;        /* Core dispatch context from dbus-broker */
    CList sources;                   /* All event sources linked list */
    CList timer_sources;             /* Timer sources (non-IO) */
    CList defer_sources;             /* Deferred sources */
    CList signal_sources;            /* Signal sources */
    CList exit_sources;              /* Exit sources */
    
    atomic_t ref_count;

    bool exit_requested;
    int exit_code;

    bool watchdog_enabled;

    struct k_mutex lock;

    /* For wait/dispatch */
    bool prepared;
    int last_timeout;                /* Last timeout used in poll */

    /* Default event loop */
    struct sd_event *default_event;
};

/* Static default event loop */
static struct sd_event *default_event_loop = NULL;
static struct k_mutex default_event_mutex;

/* ============================================================
 * Helper Functions
 * ============================================================ */

static inline uint64_t k_ticks_to_usec(int64_t ticks)
{
    return (uint64_t)k_ticks_to_ns_floor64(ticks) / 1000ULL;
}

static inline int64_t usec_to_k_ticks(uint64_t usec)
{
    return k_ns_to_ticks_floor64((int64_t)usec * 1000LL);
}

static uint64_t get_time_usec(int clock_id)
{
    (void)clock_id; /* Zephyr primarily uses monotonic clock */
    return k_ticks_to_usec(k_uptime_ticks());
}

/* Zephyr-specific time conversion */
static uint64_t clock_gettime_monotonic_us(void)
{
    return k_uptime_get() * USEC_PER_MSEC;
}

/* Zephyr poll wrapper for dispatch_context */
static int zephyr_event_poll(DispatchContext *ctx, int timeout)
{
    /* Use the existing dispatch_context_poll which already handles Zephyr */
    return dispatch_context_poll(ctx, timeout);
}

static int event_lock(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }
    k_mutex_lock(&event->lock, K_FOREVER);
    return 0;
}

static int event_unlock(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }
    k_mutex_unlock(&event->lock);
    return 0;
}

/* ============================================================
 * Event Source Reference Counting
 * ============================================================ */

sd_event_source *sd_event_source_ref(sd_event_source *source)
{
    if (!source) {
        return NULL;
    }
    atomic_inc(&source->ref_count);
    return source;
}

sd_event_source *sd_event_source_unref(sd_event_source *source)
{
    if (!source) {
        return NULL;
    }

    if (atomic_dec(&source->ref_count) == 1) {
        /* Last reference, cleanup */
        if (source->destroy_callback) {
            source->destroy_callback(source->userdata);
        }

        if (source->event) {
            event_lock(source->event);
            
            /* Unlink from appropriate list based on type */
            c_list_unlink(&source->link);
            
            /* For IO and TIME sources, deinit dispatch_file */
            if (source->type == SOURCE_IO || source->type == SOURCE_TIME) {
                LOG_DBG("[sd-event] sd_event_source_unref: calling dispatch_file_deinit for source=%p (type=%d)", 
                        source, source->type);
                dispatch_file_deinit(&source->dispatch_file);
            }
            
            event_unlock(source->event);
        }

        k_free(source->description);

        /* Type-specific cleanup */
        switch (source->type) {
        case SOURCE_TIME:
            k_work_cancel_delayable(&source->data.time.work);
            /* Close notification pipes if they exist */
            if (source->data.time.notify_pipe[0] >= 0) {
                close(source->data.time.notify_pipe[0]);
                close(source->data.time.notify_pipe[1]);
            }
            break;
        case SOURCE_SIGNAL:
            /* Nothing to cleanup for poll signal */
            break;
        case SOURCE_IO:
            /* dispatch_file already cleaned up above */
            break;
        default:
            break;
        }

        free(source);  // Use free() to match calloc()
    }

    return NULL;
}

/* ============================================================
 * Event Loop Management
 * ============================================================ */

int sd_event_new(sd_event **event)
{
    if (!event) {
        return -EINVAL;
    }

    sd_event *e = calloc(1, sizeof(sd_event));
    if (!e) {
        return -ENOMEM;
    }

    /* Initialize dispatch context */
    int r = dispatch_context_init(&e->dispatch);
    if (r < 0) {
        free(e);
        return r;
    }

    /* Initialize source lists */
    e->sources = (CList)C_LIST_INIT(e->sources);
    e->timer_sources = (CList)C_LIST_INIT(e->timer_sources);
    e->defer_sources = (CList)C_LIST_INIT(e->defer_sources);
    e->signal_sources = (CList)C_LIST_INIT(e->signal_sources);
    e->exit_sources = (CList)C_LIST_INIT(e->exit_sources);
    
    atomic_set(&e->ref_count, 1);
    e->exit_requested = false;
    e->exit_code = 0;
    e->watchdog_enabled = false;
    e->prepared = false;
    e->last_timeout = -1;

    k_mutex_init(&e->lock);

    *event = e;

    return 0;
}

int sd_event_default(sd_event **event)
{
    if (!event) {
        return -EINVAL;
    }

    k_mutex_lock(&default_event_mutex, K_FOREVER);

    if (!default_event_loop) {
        int ret = sd_event_new(&default_event_loop);
        if (ret < 0) {
            k_mutex_unlock(&default_event_mutex);
            return ret;
        }
    }

    *event = sd_event_ref(default_event_loop);
    k_mutex_unlock(&default_event_mutex);

    return 0;
}

sd_event *sd_event_ref(sd_event *event)
{
    if (!event) {
        return NULL;
    }
    atomic_inc(&event->ref_count);
    return event;
}

sd_event *sd_event_unref(sd_event *event)
{
    if (!event) {
        return NULL;
    }

    if (atomic_dec(&event->ref_count) == 1) {
        /* Remove from default if needed */
        k_mutex_lock(&default_event_mutex, K_FOREVER);
        if (event == default_event_loop) {
            default_event_loop = NULL;
        }
        k_mutex_unlock(&default_event_mutex);

        /* Cleanup all sources */
        event_lock(event);
        
        LOG_DBG("[sd-event] sd_event_unref: cleaning up all sources, sources=%d, timer=%d, defer=%d, signal=%d, exit=%d", 
                c_list_is_empty(&event->sources), c_list_is_empty(&event->timer_sources), 
                c_list_is_empty(&event->defer_sources), c_list_is_empty(&event->signal_sources),
                c_list_is_empty(&event->exit_sources));
        
        /* Use CList iteration to cleanup all sources from sources list (IO sources) */
        CList *iter, *safe;
        c_list_for_each_safe(iter, safe, &event->sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            /* First unlink the source */
            c_list_unlink(&source->link);
            
            /* Then unref - the source will clean itself up */
            sd_event_source_unref(source);
        }
        
        /* Also cleanup timer sources */
        c_list_for_each_safe(iter, safe, &event->timer_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            /* First unlink the source */
            c_list_unlink(&source->link);
            
            /* Then unref - the source will clean itself up */
            sd_event_source_unref(source);
        }
        
        /* Also cleanup defer sources */
        c_list_for_each_safe(iter, safe, &event->defer_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            /* First unlink the source */
            c_list_unlink(&source->link);
            
            /* Then unref - the source will clean itself up */
            sd_event_source_unref(source);
        }
        
        /* Also cleanup signal sources */
        c_list_for_each_safe(iter, safe, &event->signal_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            /* First unlink the source */
            c_list_unlink(&source->link);
            
            /* Then unref - the source will clean itself up */
            sd_event_source_unref(source);
        }
        
        /* Also cleanup exit sources */
        c_list_for_each_safe(iter, safe, &event->exit_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            /* First unlink the source */
            c_list_unlink(&source->link);
            
            /* Then unref - the source will clean itself up */
            sd_event_source_unref(source);
        }
        
        event_unlock(event);

        /* Deinitialize dispatch context */
        dispatch_context_deinit(&event->dispatch);

        free(event);  // Use free() to match calloc()
    } else {
        /* Reference count still in use, don't free */
    }

    return NULL;
}

int sd_event_exit(sd_event *event, int code)
{
    if (!event) {
        return -EINVAL;
    }

    event_lock(event);
    event->exit_requested = true;
    event->exit_code = code;
    
    /* Wake up the poll thread by writing to terminate_pipe */
    dispatch_context_terminate(&event->dispatch);
    
    event_unlock(event);

    return 0;
}

int sd_event_get_exit_code(sd_event *event, int *code)
{
    // if (!event || !code) {
    //     return -EINVAL;
    // }

    // event_lock(event);
    // if (!event->exit_requested) {
    //     event_unlock(event);
    //     return -ENODATA;
    // }
    // *code = event->exit_code;
    // event_unlock(event);

    return 0;
}

int sd_event_get_watchdog(sd_event *event)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);
    // int enabled = event->watchdog_enabled ? 1 : 0;
    // event_unlock(event);

    //return enabled;
    return 0;
}

int sd_event_set_watchdog(sd_event *event, int b)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);
    // event->watchdog_enabled = (b != 0);
    // event_unlock(event);

    // LOG_DBG("Watchdog %s for event loop %p", b ? "enabled" : "disabled", event);
    return 0;
}

int sd_event_get_fd(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }
    
    /* 
     * Return the read end of the terminate pipe.
     * This FD can be used to monitor the event loop from external poll/epoll.
     * When events are pending, this FD will be readable.
     */
    return event->dispatch.terminate_pipe[0];
}

int sd_event_get_state(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }
    
    /* Map internal state to systemd-compatible state values */
    if (event->exit_requested) {
        return SD_EVENT_STATE_EXITING;
    }
    
    if (event->prepared) {
        return SD_EVENT_STATE_PREPARING;
    }
    
    /* When waiting in sd_event_wait, we're in ARMED state */
    return SD_EVENT_STATE_ARMED;
}

int sd_event_now(sd_event *event, int clock, uint64_t *usec)
{
    // (void)event;

    // if (!usec) {
    //     return -EINVAL;
    // }

    // *usec = get_time_usec(clock);
    return 0;
}

/**
 * @brief Set the dispatch context for the event loop
 * @param event Event loop
 * @param dispatch External dispatch context to use (NULL to use internal)
 * @return 0 on success, negative errno on error
 * 
 * This allows sharing a dispatch context with other components (e.g., dbus-broker)
 * to avoid conflicts and improve performance.
 */
int sd_event_set_dispatch_context(sd_event *event, DispatchContext *dispatch)
{
    if (!event) {
        return -EINVAL;
    }
    
    /* If dispatch is NULL, use internal dispatch context */
    if (!dispatch) {
        /* Already using internal dispatch */
        return 0;
    }
    
    /* 
     * For now, we just store the pointer.
     * In a full implementation, you would need to:
     * 1. Deinitialize the internal dispatch context
     * 2. Use the external one for all operations
     */
    // event->dispatch = *dispatch;  // Copy or reference?
    
#ifdef __ZEPHYR__
    // LOG_DBG("Event loop %p configured to use external dispatch context %p", 
    //         event, dispatch);
#endif
    return 0;
}

/* ============================================================
 * Timer Callback Handler with Notification Pipe
 * ============================================================ */

static void timer_expiry_handler(struct k_timer *timer)
{
    sd_event_source *source = CONTAINER_OF(timer, sd_event_source, data.time.timer);

    if (!source || !source->event) {
        return;
    }

    if (source->enabled == SD_EVENT_OFF) {
        return;
    }

    source->pending = true;
    source->data.time.expired = true;

    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }

    /* Write to notification pipe to wake up poll */
    if (source->data.time.notify_pipe[1] >= 0) {
        char byte = 1;
        ssize_t ret = write(source->data.time.notify_pipe[1], &byte, 1);
    }
}

static void timer_work_handler(struct k_work *work)
{
    struct k_work_delayable *work_delayable = CONTAINER_OF(work, struct k_work_delayable, work);
    sd_event_source *source = CONTAINER_OF(work_delayable, sd_event_source, data.time.work);
    
    // LOG_DBG("[sd-event] timer_work_handler called for source %p", source);
    
    if (source->enabled == SD_EVENT_OFF) {
        // LOG_DBG("[sd-event] timer_work_handler: source is disabled");
        return;
    }
    
    /* Mark as pending */
    source->pending = true;
    source->data.time.expired = true;
    
    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }

    /* Write to notification pipe to wake up poll */
    if (source->data.time.notify_pipe[1] >= 0) {
        char byte = 1;
        ssize_t ret = write(source->data.time.notify_pipe[1], &byte, 1);
    }
}

/* Callback when timer notify_pipe becomes readable - called by dispatch_context */
static int timer_notify_dispatch(DispatchFile *file)
{
    sd_event_source *source = CONTAINER_OF(file, sd_event_source, dispatch_file);
    
    // LOG_DBG("[sd-event] timer_notify_dispatch called for source %p", source);
    
    /* Drain the notification pipe to clear the readable state */
    char buffer[8];
    while (read(source->data.time.notify_pipe[0], buffer, sizeof(buffer)) > 0) {
        /* Keep reading until empty - non-blocking read */
    }
    
    /* Mark as pending so it will be dispatched in the next dispatch phase */
    source->pending = true;
    source->data.time.expired = true;
    
    // LOG_DBG("[sd-event] timer_notify_dispatch: marked source %p as pending", source);
    
    return 0;
}

/* ============================================================
 * Work Callback Handlers (Defer/Post/Exit)
 * ============================================================ */

static void defer_work_handler(struct k_work *work)
{
    sd_event_source *source = CONTAINER_OF(work, sd_event_source, data.defer.work);

    if (source->enabled == SD_EVENT_OFF) {
        return;
    }

    if (source->data.defer.handler) {
        source->data.defer.handler(source, source->userdata);
    }

    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }
}

static void post_work_handler(struct k_work *work)
{
    sd_event_source *source = CONTAINER_OF(work, sd_event_source, data.post.work);

    if (source->enabled == SD_EVENT_OFF) {
        return;
    }

    if (source->data.post.handler) {
        source->data.post.handler(source, source->userdata);
    }

    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }
}

static void exit_work_handler(struct k_work *work)
{
    sd_event_source *source = CONTAINER_OF(work, sd_event_source, data.exit.work);

    if (source->enabled == SD_EVENT_OFF) {
        return;
    }

    if (source->data.exit.handler) {
        source->data.exit.handler(source, source->userdata);
    }

    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }
}

/* ============================================================
 * Event Source Creation Helpers
 * ============================================================ */

static void source_insert_sorted(sd_event *event, sd_event_source *source) {
    CList *i;
    
    /* Insert source in priority order (lower number = higher priority) */
    c_list_for_each(i, &event->sources) {
        sd_event_source *other = c_list_entry(i, sd_event_source, link);
        if (other->priority >= source->priority) {
            c_list_link_before(&other->link, &source->link);
            return;
        }
    }
    c_list_link_tail(&event->sources, &source->link);
}

static sd_event_source *source_new(sd_event *event, enum sd_event_source_type type)
{
    sd_event_source *source = calloc(1, sizeof(sd_event_source));
    if (!source) {
        return NULL;
    }

    source->type = type;
    source->event = event;
    source->enabled = SD_EVENT_ON;
    source->priority = SD_EVENT_DEFAULT_PRIORITY;
    source->floating = false;
    source->pending = false;
    atomic_set(&source->ref_count, 1);
    
    /* Initialize the link */
    source->link = (CList)C_LIST_INIT(source->link);

    /* Only IO sources go into the main sources list for prepare callbacks */
    if (type == SOURCE_IO) {
        /* Insert into sorted list by priority */
        source_insert_sorted(event, source);
    }

    return source;
}

/* IO handler wrapper for dispatch_context */
static int io_dispatch_wrapper(DispatchFile *file) {
    sd_event_source *source = CONTAINER_OF(file, sd_event_source, dispatch_file);
    
    if (!source || !source->data.io.handler) {
        return 0;
    }
    
    int ret = source->data.io.handler(source, 
                                 source->data.io.fd,
                                 file->events & file->user_mask,
                                 source->userdata);
    
    /* Clear events after handling */
    dispatch_file_clear(file, file->events);
    
    return ret;
}

int sd_event_add_io(sd_event *event, sd_event_source **source,
                    int fd, uint32_t events,
                    sd_event_io_handler_t callback, void *userdata)
{
    if (!event || !source || fd < 0 || !callback) {
        return -EINVAL;
    }

    event_lock(event);

    sd_event_source *s = source_new(event, SOURCE_IO);
    if (!s) {
        event_unlock(event);
        return -ENOMEM;
    }

    s->data.io.fd = fd;
    s->data.io.events = events;
    s->data.io.revents = 0;
    s->data.io.handler = callback;
    s->userdata = userdata;

    /* Initialize dispatch_file and register with dispatch_context */
    int r = dispatch_file_init(&s->dispatch_file,
                               &event->dispatch,
                               io_dispatch_wrapper,
                               fd,
                               events,
                               0);
    if (r < 0) {
        k_free(s);
        event_unlock(event);
        return r;
    }

    *source = s;
    event_unlock(event);

    return 0;
}

int sd_event_add_time(sd_event *event, sd_event_source **source,
                      int clock, uint64_t usec, uint64_t accuracy,
                      sd_event_time_handler_t callback, void *userdata)
{
    if (!event || !source || !callback) {
        return -EINVAL;
    }

    event_lock(event);

    sd_event_source *s = source_new(event, SOURCE_TIME);
    if (!s) {
        event_unlock(event);
        return -ENOMEM;
    }

    s->data.time.clock = clock;
    s->data.time.usec = usec;
    s->data.time.accuracy = accuracy ? accuracy : DEFAULT_ACCURACY_USEC;
    s->data.time.handler = callback;
    s->userdata = userdata;
    
    /* Initialize notification pipe to invalid state first */
    s->data.time.notify_pipe[0] = -1;
    s->data.time.notify_pipe[1] = -1;
    
    /* Initialize notification pipe */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, s->data.time.notify_pipe) < 0) {
        k_free(s);
        event_unlock(event);
        return -errno;
    }

    /* Set read end to non-blocking mode */
    int flags = fcntl(s->data.time.notify_pipe[0], F_GETFL, 0);
    if (flags >= 0) {
        fcntl(s->data.time.notify_pipe[0], F_SETFL, flags | O_NONBLOCK);
    }

    /* Initialize delayed work for timer (runs in worker thread, not ISR) */
    k_work_init_delayable(&s->data.time.work, timer_work_handler);

    /* Calculate absolute expiry time and relative timeout */
    uint64_t now = clock_gettime_monotonic_us();
    uint64_t absolute_expiry = now + usec;  /* usec is relative timeout */
    uint64_t timeout = usec;

    /* Store absolute expiry time for later comparison */
    s->data.time.usec = absolute_expiry;

    k_work_schedule(&s->data.time.work, K_USEC(timeout));

    /* Register notification pipe read-end with dispatch_context for monitoring */
    int r = dispatch_file_init(&s->dispatch_file, &event->dispatch, timer_notify_dispatch, 
                               s->data.time.notify_pipe[0], EPOLLIN, 0);
    if (r < 0) {
        close(s->data.time.notify_pipe[0]);
        close(s->data.time.notify_pipe[1]);
        k_free(s);
        event_unlock(event);
        return r;
    }
    
    /* Enable EPOLLIN notification in user_mask - CRITICAL for poll to monitor this fd */
    dispatch_file_select(&s->dispatch_file, EPOLLIN);
    
    // LOG_DBG("[sd-event] Registered notify_pipe[0]=%d with dispatch_context (user_mask=EPOLLIN)", s->data.time.notify_pipe[0]);

    /* Add to timer_sources list for processing */
    c_list_link_tail(&event->timer_sources, &s->link);

    *source = s;
    event_unlock(event);

    return 0;
}

int sd_event_add_time_relative(sd_event *event, sd_event_source **source,
                               int clock, uint64_t usec, uint64_t accuracy,
                               sd_event_time_handler_t callback, void *userdata)
{
    return 0;
}

int sd_event_add_signal(sd_event *event, sd_event_source **source,
                        int sig, sd_event_signal_handler_t callback,
                        void *userdata)
{
    // if (!event || !source || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_SIGNAL);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.signal.sig = sig;
    // s->data.signal.handler = callback;
    // s->userdata = userdata;

    // k_poll_signal_init(&s->data.signal.signal);

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added signal source %p (sig=%d)", s, sig);
    return 0;
}

int sd_event_add_child(sd_event *event, sd_event_source **source,
                       pid_t pid, int options,
                       sd_event_child_handler_t callback, void *userdata)
{
    /* Zephyr doesn't have processes, so this is a stub */
    // (void)event;
    // (void)source;
    // (void)pid;
    // (void)options;
    // (void)callback;
    // (void)userdata;

    // LOG_WRN("Child process monitoring not supported on Zephyr");
    return -ENOTSUP;
}

static void defer_source_insert_sorted(sd_event *event, sd_event_source *source) {
    CList *i;
    
    /* Insert source in priority order (lower number = higher priority) */
    /* Higher priority sources (smaller numbers) should come FIRST */
    /* For same priority, maintain FIFO order (append after all same-priority sources) */
    c_list_for_each(i, &event->defer_sources) {
        sd_event_source *other = c_list_entry(i, sd_event_source, link);
        
        /* Insert before the first source that has STRICTLY LOWER priority (larger number) */
        /* Use <= to skip all sources with same or higher priority */
        if (source->priority <= other->priority) {
            /* Skip sources with same priority to maintain FIFO order */
            if (source->priority == other->priority) {
                continue;  /* Continue to find next different priority */
            }
            c_list_link_before(&other->link, &source->link);
            goto done;
        }
    }
    /* If all sources have higher or equal priority, append to tail */
    c_list_link_tail(&event->defer_sources, &source->link);
    
done:
    /* Print final list state */
    CList *iter;
    c_list_for_each(iter, &event->defer_sources) {
        sd_event_source *src = c_list_entry(iter, sd_event_source, link);
    }
}

int sd_event_add_defer(sd_event *event, sd_event_source **source,
                       sd_event_handler_t callback, void *userdata)
{
    if (!event || !source || !callback) {
        return -EINVAL;
    }

    event_lock(event);

    sd_event_source *s = source_new(event, SOURCE_DEFER);
    if (!s) {
        event_unlock(event);
        return -ENOMEM;
    }

    s->data.defer.handler = callback;
    s->userdata = userdata;

    k_work_init(&s->data.defer.work, defer_work_handler);

    /* Defer sources are oneshot by default */
    s->enabled = SD_EVENT_ONESHOT;

    /* Add to defer_sources list sorted by priority */
    defer_source_insert_sorted(event, s);

    *source = s;
    
    /* Print current list state in one line */
    CList *iter;
    c_list_for_each(iter, &event->defer_sources) {
        sd_event_source *src = c_list_entry(iter, sd_event_source, link);
    }
    
    event_unlock(event);

    return 0;
}

int sd_event_add_post(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata)
{
    if (!event || !source || !callback) {
        return -EINVAL;
    }

    event_lock(event);

    sd_event_source *s = source_new(event, SOURCE_POST);
    if (!s) {
        event_unlock(event);
        return -ENOMEM;
    }

    s->data.post.handler = callback;
    s->userdata = userdata;

    k_work_init(&s->data.post.work, post_work_handler);

    *source = s;
    event_unlock(event);

    return 0;
}

int sd_event_add_exit(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata)
{
    if (!event || !callback) {
        return -EINVAL;
    }

    event_lock(event);

    sd_event_source *s = source_new(event, SOURCE_EXIT);
    if (!s) {
        event_unlock(event);
        return -ENOMEM;
    }

    s->data.exit.handler = callback;
    s->userdata = userdata;

    k_work_init(&s->data.exit.work, exit_work_handler);

    /* Add to exit_sources list */
    c_list_link_tail(&event->exit_sources, &s->link);

    if (source) {
        *source = s;
    }
    event_unlock(event);

    return 0;
}

/* ============================================================
 * Event Source Getters/Setters
 * ============================================================ */

void *sd_event_source_get_userdata(sd_event_source *source)
{
    if (!source) {
        return NULL;
    }
    return source->userdata;
}

void *sd_event_source_set_userdata(sd_event_source *source, void *userdata)
{
    if (!source) {
        return NULL;
    }
    void *old = source->userdata;
    source->userdata = userdata;
    return old;
}

int sd_event_source_get_description(sd_event_source *source, const char **description)
{
    // if (!source || !description) {
    //     return -EINVAL;
    // }
    // *description = source->description;
    return 0;
}

int sd_event_source_set_description(sd_event_source *source, const char *description)
{
    // if (!source) {
    //     return -EINVAL;
    // }

    // k_free(source->description);
    // source->description = NULL;

    // if (description) {
    //     source->description = (description);
    //     if (!source->description) {
    //         return -ENOMEM;
    //     }
    // }

    return 0;
}

int sd_event_source_set_prepare(sd_event_source *source, sd_event_handler_t callback)
{
    // if (!source) {
    //     return -EINVAL;
    // }
    // source->prepare_callback = callback;
    return 0;
}

sd_event* sd_event_source_get_event(sd_event_source *source)
{
    if (!source) {
        return NULL;
    }
    return source->event;
}

int sd_event_source_get_pending(sd_event_source *source)
{
    // if (!source) {
    //     return -EINVAL;
    // }
    // return source->pending ? 1 : 0;
    return 0;
}

int sd_event_source_get_priority(sd_event_source *source, int64_t *priority)
{
    // if (!source || !priority) {
    //     return -EINVAL;
    // }
    // *priority = source->priority;
    return 0;
}

int sd_event_source_set_priority(sd_event_source *source, int64_t priority)
{
    if (!source) {
        return -EINVAL;
    }
    
    /* If source is already in a list, we need to re-sort it */
    if (c_list_is_linked(&source->link)) {
        sd_event *event = source->event;
        event_lock(event);
        
        /* Unlink from current position */
        c_list_unlink(&source->link);
        
        /* Update priority */
        source->priority = priority;
        
        /* Re-insert in sorted order */
        /* Check which list this source belongs to by type */
        if (source->type == SOURCE_DEFER) {
            defer_source_insert_sorted(event, source);
        } else {
            source_insert_sorted(event, source);
        }
        
        event_unlock(event);
    } else {
        /* Not linked yet, just set priority */
        source->priority = priority;
    }
    
    return 0;
}

int sd_event_source_get_enabled(sd_event_source *source, int *enabled)
{
    if (!source || !enabled) {
        return -EINVAL;
    }
    *enabled = source->enabled;
    return 0;
}

int sd_event_source_set_enabled(sd_event_source *source, int enabled)
{
    if (!source) {
        return -EINVAL;
    }

    if (enabled < SD_EVENT_OFF || enabled > SD_EVENT_ONESHOT) {
        return -EINVAL;
    }

    source->enabled = enabled;

    /* Handle timer restart for time sources */
    if (source->type == SOURCE_TIME && enabled == SD_EVENT_ON) {
        uint64_t now = clock_gettime_monotonic_us();
        uint64_t timeout = (source->data.time.usec > now) ?
                           (source->data.time.usec - now) : 0;
        
        k_timer_start(&source->data.time.timer,
                      K_USEC(timeout),
                      K_NO_WAIT);
    }
    
    /* For IO sources, select/deselect events in dispatch_context */
    if (source->type == SOURCE_IO) {
        if (enabled == SD_EVENT_ON) {
            /* Select events for monitoring */
            dispatch_file_select(&source->dispatch_file, source->data.io.events);
        } else if (enabled == SD_EVENT_OFF) {
            /* Deselect all events */
            dispatch_file_deselect(&source->dispatch_file, source->data.io.events);
        }
    }

    return 0;
}

int sd_event_source_get_floating(sd_event_source *source)
{
    // if (!source) {
    //     return -EINVAL;
    // }
    // return source->floating ? 1 : 0;
    return 0;
}

int sd_event_source_set_floating(sd_event_source *source, int b)
{
    // if (!source) {
    //     return -EINVAL;
    // }
    // source->floating = (b != 0);
    return 0;
}

int sd_event_source_set_destroy_callback(sd_event_source *source,
                                          sd_event_destroy_t callback)
{
    // if (!source) {
    //     return -EINVAL;
    // }
    // source->destroy_callback = callback;
    return 0;
}

int sd_event_source_get_destroy_callback(sd_event_source *source,
                                          sd_event_destroy_t *callback)
{
    // if (!source || !callback) {
    //     return -EINVAL;
    // }
    // *callback = source->destroy_callback;
    return 0;
}

/* ============================================================
 * IO Source Specific
 * ============================================================ */

int sd_event_source_get_io_fd(sd_event_source *source)
{
    // if (!source || source->type != SOURCE_IO) {
    //     return -EINVAL;
    // }
    // return source->data.io.fd;
    return 0;
}

int sd_event_source_set_io_fd(sd_event_source *source, int fd)
{
    // if (!source || source->type != SOURCE_IO || fd < 0) {
    //     return -EINVAL;
    // }
    // source->data.io.fd = fd;
    return 0;
}

int sd_event_source_get_io_events(sd_event_source *source, uint32_t *events)
{
    // if (!source || source->type != SOURCE_IO || !events) {
    //     return -EINVAL;
    // }
    // *events = source->data.io.events;
    return 0;
}

int sd_event_source_set_io_events(sd_event_source *source, uint32_t events)
{
    // if (!source || source->type != SOURCE_IO) {
    //     return -EINVAL;
    // }
    // source->data.io.events = events;
    return 0;
}

int sd_event_source_get_io_revents(sd_event_source *source, uint32_t *revents)
{
    // if (!source || source->type != SOURCE_IO || !revents) {
    //     return -EINVAL;
    // }
    // *revents = source->data.io.revents;
    return 0;
}

/* ============================================================
 * Time Source Specific
 * ============================================================ */

int sd_event_source_get_time(sd_event_source *source, uint64_t *usec)
{
    // if (!source || source->type != SOURCE_TIME || !usec) {
    //     return -EINVAL;
    // }
    // *usec = source->data.time.usec;
    return 0;
}

int sd_event_source_set_time(sd_event_source *source, uint64_t usec)
{
    if (!source || source->type != SOURCE_TIME) {
        return -EINVAL;
    }
    source->data.time.usec = usec;
    
    /* Clear pending flag when setting new time - ensures timer is re-evaluated */
    source->pending = false;
    source->data.time.expired = false;

    /* Restart timer with new time */
    if (source->enabled == SD_EVENT_ON) {
        uint64_t now = clock_gettime_monotonic_us();
        uint64_t timeout = (usec > now) ? (usec - now) : 0;
        
        /* Restart both k_timer and k_work_delayable to ensure timer fires correctly */
        k_timer_start(&source->data.time.timer,
                      K_USEC(timeout),
                      K_NO_WAIT);
        
        /* Re-schedule the delayed work which actually triggers the expiration logic */
        k_work_reschedule(&source->data.time.work, K_USEC(timeout));
    }

    return 0;
}

int sd_event_source_get_time_accuracy(sd_event_source *source, uint64_t *usec)
{
    // if (!source || source->type != SOURCE_TIME || !usec) {
    //     return -EINVAL;
    // }
    // *usec = source->data.time.accuracy;
    return 0;
}

int sd_event_source_set_time_accuracy(sd_event_source *source, uint64_t usec)
{
    // if (!source || source->type != SOURCE_TIME) {
    //     return -EINVAL;
    // }
    // source->data.time.accuracy = usec;
    return 0;
}

/* ============================================================
 * Signal Source Specific
 * ============================================================ */

int sd_event_source_get_signal(sd_event_source *source)
{
    // if (!source || source->type != SOURCE_SIGNAL) {
    //     return -EINVAL;
    // }
    // return source->data.signal.sig;
    return 0;
}

/* ============================================================
 * Child Source Specific
 * ============================================================ */

int sd_event_source_get_child_pid(sd_event_source *source, pid_t *pid)
{
    // if (!source || source->type != SOURCE_CHILD || !pid) {
    //     return -EINVAL;
    // }
    // *pid = source->data.child.pid;
    return 0;
}

/* ============================================================
 * Event Loop Iteration (Prepare/Wait/Dispatch)
 * ============================================================ */

/**
 * @brief Process timer sources and check for expired timers
 */
static void process_timer_sources(sd_event *event) {
    CList *iter;
    
    // Check if list is empty
    if (event->timer_sources.next == &event->timer_sources) {
        return;
    }
    
    c_list_for_each(iter, &event->timer_sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        
        if (!source) {
            continue;
        }
        
        if (source->enabled == SD_EVENT_OFF) {
            continue;
        }
        
        /* Check if timer has expired (set by timer_work_handler) */
        if (source->data.time.expired && !source->pending) {
            source->pending = true;
            // LOG_DBG("[sd-event] Timer %p marked as pending (expired flag set)", source);
            
            if (source->enabled == SD_EVENT_ONESHOT) {
                source->enabled = SD_EVENT_OFF;
            }
        }
        
        /* Also check notification pipe for compatibility - drain any pending data */
        char buffer[8];
        ssize_t n = read(source->data.time.notify_pipe[0], buffer, sizeof(buffer));
        
        // LOG_DBG("[sd-event] process_timer_sources: source=%p, read from pipe returned %d", source, (int)n);
        
        if (n > 0) {
            source->pending = true;
            // LOG_DBG("[sd-event] Timer %p marked as pending (read %d bytes from pipe)", source, (int)n);
            
            if (source->enabled == SD_EVENT_ONESHOT) {
                source->enabled = SD_EVENT_OFF;
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            /* Real error, not just "no data yet" */
            // printk("[sd-event] Read error from notify_pipe[%d]: errno=%d\n", 
            //        source->data.time.notify_pipe[0], errno);
        }
    }
}

/**
 * @brief Process deferred sources
 */
static void process_deferred_sources(sd_event *event) {
    CList *iter, *safe;
    
    /* Mark all enabled deferred sources as pending */
    c_list_for_each_safe(iter, safe, &event->defer_sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        
        if (source->enabled == SD_EVENT_OFF) {
            continue;
        }
        
        /* Mark as pending for dispatch */
        source->pending = true;
    }
}

int sd_event_prepare(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }

    event_lock(event);

    /* Call prepare callbacks for all sources */
    CList *iter;
    c_list_for_each(iter, &event->sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        
        if (source->enabled != SD_EVENT_OFF && source->prepare_callback) {
            source->prepare_callback(source, source->userdata);
        }
    }

    /* Process timer sources */
    process_timer_sources(event);
    
    /* Process deferred sources */
    process_deferred_sources(event);

    event->prepared = true;
    event_unlock(event);

    return 0;
}

int sd_event_wait(sd_event *event, uint64_t usec)
{
    if (!event) {
        return -EINVAL;
    }

    event_lock(event);

    if (!event->prepared) {
        event_unlock(event);
        int ret = sd_event_prepare(event);
        if (ret < 0) {
            return ret;
        }
        event_lock(event);
    }

    /* Check if there are any pending non-IO sources (deferred, timers, etc.) */
    bool has_pending = false;
    
    /* Check deferred sources */
    CList *iter;
    c_list_for_each(iter, &event->defer_sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        if (source->pending && source->enabled != SD_EVENT_OFF) {
            has_pending = true;
            break;
        }
    }
    
    /* Check timer sources */
    if (!has_pending) {
        c_list_for_each(iter, &event->timer_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            if (source->pending && source->enabled != SD_EVENT_OFF) {
                has_pending = true;
                break;
            }
        }
    }

    /* Calculate the earliest timer timeout */
    int64_t timer_timeout_ms = -1;
    
    if (!has_pending && usec == (uint64_t)-1) {
        /* Only calculate timer timeout if user didn't specify a timeout */
        uint64_t now = clock_gettime_monotonic_us();
        
        c_list_for_each(iter, &event->timer_sources) {
            sd_event_source *source = c_list_entry(iter, sd_event_source, link);
            
            if (source->enabled == SD_EVENT_OFF) {
                continue;
            }
            
            uint64_t target_usec = source->data.time.usec;
            
            if (target_usec > now) {
                int64_t remaining_usec = (int64_t)(target_usec - now);
                int64_t remaining_ms = remaining_usec / 1000;
                
                if (remaining_ms > 0 && (timer_timeout_ms < 0 || remaining_ms < timer_timeout_ms)) {
                    timer_timeout_ms = (int)remaining_ms;
                }
            } else {
                /* Timer already expired - should have been handled, but use small timeout to avoid blocking */
                if (timer_timeout_ms < 0 || timer_timeout_ms > 1) {
                    timer_timeout_ms = 1;  // Use 1ms timeout to avoid infinite blocking
                }
            }
        }
    }

    /* Calculate timeout */
    int timeout_ms;
    if (has_pending) {
        /* If there are pending sources, don't block in poll */
        timeout_ms = 0;
    } else if (timer_timeout_ms >= 0) {
        /* Use calculated timer timeout */
        timeout_ms = (int)timer_timeout_ms;
    } else if (usec == (uint64_t)-1) {
        timeout_ms = -1; /* Infinite */
    } else {
        timeout_ms = (int)(usec / 1000);
        if (timeout_ms < 0) {
            timeout_ms = 0;
        }
    }

    event->last_timeout = timeout_ms;
    
    LOG_DBG("[sd-event] sd_event_wait: has_pending=%d, timer_timeout_ms=%lld, usec=%llu, final timeout_ms=%d", 
            has_pending, (long long)timer_timeout_ms, (unsigned long long)usec, timeout_ms);
    
    event_unlock(event);

    /* Use dispatch_context_poll to wait for events */
    int ret = dispatch_context_poll(&event->dispatch, timeout_ms);
    
    LOG_DBG("[sd-event] sd_event_wait: dispatch_context_poll returned %d", ret);
    
    return ret;
}

int sd_event_dispatch(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }

    event_lock(event);

    /* First, handle deferred sources (before checking exit) */
    CList *iter, *safe;
    
    c_list_for_each_safe(iter, safe, &event->defer_sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        
        if (!source->pending || source->enabled == SD_EVENT_OFF) {
            continue;
        }

        source->pending = false;
        
        if (source->data.defer.handler) {
            event_unlock(event);
            source->data.defer.handler(source, source->userdata);
            event_lock(event);
        }
        
        if (source->enabled == SD_EVENT_ONESHOT) {
            source->enabled = SD_EVENT_OFF;
        }
    }

    /* Check for exit AFTER handling deferred sources */
    if (event->exit_requested) {
        event_unlock(event);
        return 0;
    }

    /* Handle timer sources */
    c_list_for_each_safe(iter, safe, &event->timer_sources) {
        sd_event_source *source = c_list_entry(iter, sd_event_source, link);
        
        if (!source || !source->data.time.handler) {
            continue;
        }
        
        if (!source->pending || source->enabled == SD_EVENT_OFF) {
            continue;
        }

        source->pending = false;
        
        uint64_t now = clock_gettime_monotonic_us();
        
        event_unlock(event);
        int ret = source->data.time.handler(source, now, source->userdata);
        event_lock(event);
        
        if (source->enabled == SD_EVENT_ONESHOT) {
            source->enabled = SD_EVENT_OFF;
        }
    }

    /* Dispatch IO sources via dispatch_context */
    int ret = dispatch_context_dispatch(&event->dispatch);
    
    event->prepared = false;
    event_unlock(event);

    return ret;
}

int sd_event_run(sd_event *event, uint64_t usec)
{
    int ret;

    ret = sd_event_prepare(event);
    if (ret < 0) {
        return ret;
    }

    ret = sd_event_wait(event, usec);
    if (ret < 0) {
        return ret;
    }

    ret = sd_event_dispatch(event);
    
    return ret;
}

int sd_event_loop(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }

    /* Execute at least one iteration to handle deferred sources */
    bool first_iteration = true;
    
    while (1) {
        /* Check for exit, but only after first iteration completes */
        event_lock(event);
        bool should_exit = event->exit_requested && !first_iteration;
        event_unlock(event);

        if (should_exit) {
            CList *iter, *safe;
            c_list_for_each_safe(iter, safe, &event->exit_sources) {
                sd_event_source *source = c_list_entry(iter, sd_event_source, link);
                
                if (source->enabled != SD_EVENT_OFF) {
                    
                    if (source->data.exit.handler) {
                        source->data.exit.handler(source, source->userdata);
                    }
                    
                    if (source->enabled == SD_EVENT_ONESHOT) {
                        source->enabled = SD_EVENT_OFF;
                    }
                }
            }
            break;
        }

        int ret = sd_event_run(event, (uint64_t)-1);
        if (ret < 0 && ret != -EINTR) {
            return ret;
        }
        
        first_iteration = false;
    }

    return event->exit_code;
}

/* ============================================================
 * Initialization
 * ============================================================ */

static int sd_event_init(void)
{
    k_mutex_init(&default_event_mutex);
    //LOG_INF("sd-event for Zephyr initialized");
    return 0;
}

SYS_INIT(sd_event_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);