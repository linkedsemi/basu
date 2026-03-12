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

//LOG_MODULE_REGISTER(sd_event, CONFIG_SD_EVENT_LOG_LEVEL);

/* Maximum number of event sources per event loop */
#ifndef CONFIG_SD_EVENT_MAX_SOURCES
#define CONFIG_SD_EVENT_MAX_SOURCES 32
#endif

/* Maximum number of IO sources for poll */
#ifndef CONFIG_SD_EVENT_MAX_IO_SOURCES
#define CONFIG_SD_EVENT_MAX_IO_SOURCES 16
#endif

/* Default priority for event sources */
#define SD_EVENT_DEFAULT_PRIORITY 0

/* ============================================================
 * Internal Structures
 * ============================================================ */

struct sd_event_source {
    sys_dnode_t node;

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
    sys_dlist_t sources;
    atomic_t ref_count;

    bool exit_requested;
    int exit_code;

    bool watchdog_enabled;

    struct k_mutex lock;

    /* For wait/dispatch */
    bool prepared;

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
    // if (!source) {
    //     return NULL;
    // }
    // atomic_inc(&source->ref_count);
    // return source;
    return NULL;
}

sd_event_source *sd_event_source_unref(sd_event_source *source)
{
    // if (!source) {
    //     return NULL;
    // }

    // if (atomic_dec(&source->ref_count) == 1) {
    //     /* Last reference, cleanup */
    //     if (source->destroy_callback) {
    //         source->destroy_callback(source->userdata);
    //     }

    //     if (source->event) {
    //         event_lock(source->event);
    //         sys_dlist_remove(&source->node);
    //         event_unlock(source->event);
    //     }

    //     k_free(source->description);

    //     /* Type-specific cleanup */
    //     switch (source->type) {
    //     case SOURCE_TIME:
    //         k_timer_stop(&source->data.time.timer);
    //         break;
    //     case SOURCE_SIGNAL:
    //         /* Nothing to cleanup for poll signal */
    //         break;
    //     default:
    //         break;
    //     }

    //     k_free(source);
    // }

    return NULL;
}

/* ============================================================
 * Event Loop Management
 * ============================================================ */

int sd_event_new(sd_event **event)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // sd_event *e = k_calloc(1, sizeof(sd_event));
    // if (!e) {
    //     return -ENOMEM;
    // }

    // sys_dlist_init(&e->sources);
    // atomic_set(&e->ref_count, 1);
    // e->exit_requested = false;
    // e->exit_code = 0;
    // e->watchdog_enabled = false;
    // e->prepared = false;

    // k_mutex_init(&e->lock);

    // *event = e;
    // LOG_DBG("Created new event loop %p", e);
    return 0;
}

int sd_event_default(sd_event **event)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // k_mutex_lock(&default_event_mutex, K_FOREVER);

    // if (!default_event_loop) {
    //     int ret = sd_event_new(&default_event_loop);
    //     if (ret < 0) {
    //         k_mutex_unlock(&default_event_mutex);
    //         return ret;
    //     }
    // }

    // *event = sd_event_ref(default_event_loop);
    // k_mutex_unlock(&default_event_mutex);

    return 0;
}

sd_event *sd_event_ref(sd_event *event)
{
    // if (!event) {
    //     return NULL;
    // }
    // atomic_inc(&event->ref_count);
    return event;
}

sd_event *sd_event_unref(sd_event *event)
{
    // if (!event) {
    //     return NULL;
    // }

    // if (atomic_dec(&event->ref_count) == 1) {
    //     /* Remove from default if needed */
    //     k_mutex_lock(&default_event_mutex, K_FOREVER);
    //     if (event == default_event_loop) {
    //         default_event_loop = NULL;
    //     }
    //     k_mutex_unlock(&default_event_mutex);

    //     /* Cleanup all sources */
    //     event_lock(event);
    //     sd_event_source *source, *next;
    //     SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&event->sources, source, next, node) {
    //         source->event = NULL; /* Prevent double removal */
    //         sd_event_source_unref(source);
    //     }
    //     event_unlock(event);

    //     k_free(event);
    //     LOG_DBG("Destroyed event loop %p", event);
    // }

    return NULL;
}

int sd_event_exit(sd_event *event, int code)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);
    // event->exit_requested = true;
    // event->exit_code = code;
    // event_unlock(event);

    // LOG_DBG("Event loop %p exit requested with code %d", event, code);
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

int sd_event_now(sd_event *event, int clock, uint64_t *usec)
{
    // (void)event;

    // if (!usec) {
    //     return -EINVAL;
    // }

    // *usec = get_time_usec(clock);
    return 0;
}

/* ============================================================
 * Timer Callback Handler
 * ============================================================ */

static void timer_expiry_handler(struct k_timer *timer)
{
    sd_event_source *source = CONTAINER_OF(timer, sd_event_source, data.time.timer);

    if (source->enabled == SD_EVENT_OFF) {
        return;
    }

    source->pending = true;

    if (source->enabled == SD_EVENT_ONESHOT) {
        source->enabled = SD_EVENT_OFF;
    }

    //LOG_DBG("Timer source %p expired", source);
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

    //LOG_DBG("Defer source %p handled", source);
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

    //LOG_DBG("Post source %p handled", source);
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

    //LOG_DBG("Exit source %p handled", source);
}

/* ============================================================
 * Event Source Creation
 * ============================================================ */

static sd_event_source *source_new(sd_event *event, enum sd_event_source_type type)
{
    sd_event_source *source = k_calloc(1, sizeof(sd_event_source));
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

    sys_dlist_append(&event->sources, &source->node);

    return source;
}

int sd_event_add_io(sd_event *event, sd_event_source **source,
                    int fd, uint32_t events,
                    sd_event_io_handler_t callback, void *userdata)
{
    // if (!event || !source || fd < 0 || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_IO);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.io.fd = fd;
    // s->data.io.events = events;
    // s->data.io.revents = 0;
    // s->data.io.handler = callback;
    // s->userdata = userdata;

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added IO source %p (fd=%d, events=0x%x)", s, fd, events);
    return 0;
}

int sd_event_add_time(sd_event *event, sd_event_source **source,
                      int clock, uint64_t usec, uint64_t accuracy,
                      sd_event_time_handler_t callback, void *userdata)
{
    // if (!event || !source || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_TIME);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.time.clock = clock;
    // s->data.time.usec = usec;
    // s->data.time.accuracy = accuracy;
    // s->data.time.handler = callback;
    // s->userdata = userdata;

    // k_timer_init(&s->data.time.timer, timer_expiry_handler, NULL);

    // /* Calculate relative timeout */
    // uint64_t now = get_time_usec(clock);
    // uint64_t timeout = (usec > now) ? (usec - now) : 0;

    // k_timer_start(&s->data.time.timer,
    //               K_USEC(timeout),
    //               K_NO_WAIT); /* One-shot, will be rearmed in handler if needed */

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added time source %p (usec=%llu)", s, usec);
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

int sd_event_add_defer(sd_event *event, sd_event_source **source,
                       sd_event_handler_t callback, void *userdata)
{
    // if (!event || !source || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_DEFER);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.defer.handler = callback;
    // s->userdata = userdata;

    // k_work_init(&s->data.defer.work, defer_work_handler);

    // /* Defer sources are oneshot by default */
    // s->enabled = SD_EVENT_ONESHOT;

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added defer source %p", s);
    return 0;
}

int sd_event_add_post(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata)
{
    // if (!event || !source || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_POST);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.post.handler = callback;
    // s->userdata = userdata;

    // k_work_init(&s->data.post.work, post_work_handler);

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added post source %p", s);
    return 0;
}

int sd_event_add_exit(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata)
{
    // if (!event || !source || !callback) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // sd_event_source *s = source_new(event, SOURCE_EXIT);
    // if (!s) {
    //     event_unlock(event);
    //     return -ENOMEM;
    // }

    // s->data.exit.handler = callback;
    // s->userdata = userdata;

    // k_work_init(&s->data.exit.work, exit_work_handler);

    // *source = s;
    // event_unlock(event);

    // LOG_DBG("Added exit source %p", s);
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
    // if (!source) {
    //     return NULL;
    // }
    // void *old = source->userdata;
    // source->userdata = userdata;
    // return old;
    return NULL;
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
    // if (!source) {
    //     return -EINVAL;
    // }
    // source->priority = priority;
    return 0;
}

int sd_event_source_get_enabled(sd_event_source *source, int *enabled)
{
    // if (!source || !enabled) {
    //     return -EINVAL;
    // }
    // *enabled = source->enabled;
    return 0;
}

int sd_event_source_set_enabled(sd_event_source *source, int enabled)
{
    // if (!source) {
    //     return -EINVAL;
    // }

    // if (enabled < SD_EVENT_OFF || enabled > SD_EVENT_ONESHOT) {
    //     return -EINVAL;
    // }

    // source->enabled = enabled;

    // /* Handle timer restart for time sources */
    // if (source->type == SOURCE_TIME && enabled == SD_EVENT_ON) {
    //     uint64_t now = get_time_usec(source->data.time.clock);
    //     uint64_t timeout = (source->data.time.usec > now) ?
    //                        (source->data.time.usec - now) : 0;
    //     k_timer_start(&source->data.time.timer,
    //                   K_USEC(timeout),
    //                   K_NO_WAIT);
    // }

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
    // if (!source || source->type != SOURCE_TIME) {
    //     return -EINVAL;
    // }
    // source->data.time.usec = usec;

    // /* Restart timer with new time */
    // if (source->enabled == SD_EVENT_ON) {
    //     uint64_t now = get_time_usec(source->data.time.clock);
    //     uint64_t timeout = (usec > now) ? (usec - now) : 0;
    //     k_timer_start(&source->data.time.timer,
    //                   K_USEC(timeout),
    //                   K_NO_WAIT);
    // }

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

int sd_event_prepare(sd_event *event)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // /* Call prepare callbacks */
    // sd_event_source *source;
    // SYS_DLIST_FOR_EACH_CONTAINER(&event->sources, source, node) {
    //     if (source->enabled != SD_EVENT_OFF && source->prepare_callback) {
    //         source->prepare_callback(source, source->userdata);
    //     }
    // }

    // event->prepared = true;
    // event_unlock(event);

    return 0;
}

int sd_event_wait(sd_event *event, uint64_t usec)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // if (!event->prepared) {
    //     event_unlock(event);
    //     sd_event_prepare(event);
    //     event_lock(event);
    // }

    // /* Build poll array for IO sources */
    // struct zsock_pollfd poll_fds[CONFIG_SD_EVENT_MAX_IO_SOURCES];
    // sd_event_source *io_sources[CONFIG_SD_EVENT_MAX_IO_SOURCES];
    // int num_io = 0;

    // sd_event_source *source;
    // SYS_DLIST_FOR_EACH_CONTAINER(&event->sources, source, node) {
    //     if (source->type == SOURCE_IO && source->enabled != SD_EVENT_OFF) {
    //         if (num_io >= CONFIG_SD_EVENT_MAX_IO_SOURCES) {
    //             break;
    //         }

    //         poll_fds[num_io].fd = source->data.io.fd;
    //         poll_fds[num_io].events = 0;

    //         if (source->data.io.events & SD_EVENT_READABLE) {
    //             poll_fds[num_io].events |= ZSOCK_POLLIN;
    //         }
    //         if (source->data.io.events & SD_EVENT_WRITABLE) {
    //             poll_fds[num_io].events |= ZSOCK_POLLOUT;
    //         }

    //         poll_fds[num_io].revents = 0;
    //         io_sources[num_io] = source;
    //         num_io++;
    //     }
    // }

    // /* Check for pending timers and signals */
    // bool has_pending = false;
    // SYS_DLIST_FOR_EACH_CONTAINER(&event->sources, source, node) {
    //     if (source->pending) {
    //         has_pending = true;
    //         break;
    //     }
    //     if (source->type == SOURCE_SIGNAL) {
    //         int signaled = 0, result = 0;
    //         k_poll_signal_check(&source->data.signal.signal, &signaled, &result);
    //         if (signaled) {
    //             source->pending = true;
    //             has_pending = true;
    //             break;
    //         }
    //     }
    // }

    // event_unlock(event);

    // /* If we have pending events, don't wait */
    // int timeout_ms = 0;
    // if (!has_pending) {
    //     if (usec == (uint64_t)-1) {
    //         timeout_ms = -1; /* Infinite */
    //     } else {
    //         timeout_ms = (int)(usec / 1000);
    //         if (timeout_ms < 0) timeout_ms = 0;
    //     }
    // }

    // /* Poll IO sources */
    // if (num_io > 0 && !has_pending) {
    //     int ret = zsock_poll(poll_fds, num_io, timeout_ms);
    //     if (ret > 0) {
    //         event_lock(event);
    //         for (int i = 0; i < num_io; i++) {
    //             if (poll_fds[i].revents != 0) {
    //                 io_sources[i]->data.io.revents = 0;
    //                 if (poll_fds[i].revents & ZSOCK_POLLIN) {
    //                     io_sources[i]->data.io.revents |= SD_EVENT_READABLE;
    //                 }
    //                 if (poll_fds[i].revents & ZSOCK_POLLOUT) {
    //                     io_sources[i]->data.io.revents |= SD_EVENT_WRITABLE;
    //                 }
    //                 if (poll_fds[i].revents & ZSOCK_POLLERR) {
    //                     io_sources[i]->data.io.revents |= SD_EVENT_ERROR;
    //                 }
    //                 if (poll_fds[i].revents & ZSOCK_POLLHUP) {
    //                     io_sources[i]->data.io.revents |= SD_EVENT_HANGUP;
    //                 }
    //                 io_sources[i]->pending = true;
    //             }
    //         }
    //         event_unlock(event);
    //     }
    // }

    return 0;
}

int sd_event_dispatch(sd_event *event)
{
    // if (!event) {
    //     return -EINVAL;
    // }

    // event_lock(event);

    // /* Check for exit */
    // if (event->exit_requested) {
    //     event_unlock(event);
    //     return 0;
    // }

    // /* Process pending sources by priority */
    // sd_event_source *source;

    // /* First, handle exit sources */
    // SYS_DLIST_FOR_EACH_CONTAINER(&event->sources, source, node) {
    //     if (source->type == SOURCE_EXIT && source->pending &&
    //         source->enabled != SD_EVENT_OFF) {
    //         source->pending = false;
    //         if (source->data.exit.handler) {
    //             event_unlock(event);
    //             source->data.exit.handler(source, source->userdata);
    //             event_lock(event);
    //         }
    //     }
    // }

    // /* Handle other pending sources */
    // SYS_DLIST_FOR_EACH_CONTAINER(&event->sources, source, node) {
    //     if (!source->pending || source->enabled == SD_EVENT_OFF) {
    //         continue;
    //     }

    //     source->pending = false;

    //     switch (source->type) {
    //     case SOURCE_IO:
    //         if (source->data.io.handler) {
    //             int fd = source->data.io.fd;
    //             uint32_t revents = source->data.io.revents;
    //             event_unlock(event);
    //             source->data.io.handler(source, fd, revents, source->userdata);
    //             event_lock(event);
    //         }
    //         if (source->enabled == SD_EVENT_ONESHOT) {
    //             source->enabled = SD_EVENT_OFF;
    //         }
    //         break;

    //     case SOURCE_TIME:
    //         if (source->data.time.handler) {
    //             uint64_t usec = get_time_usec(source->data.time.clock);
    //             event_unlock(event);
    //             source->data.time.handler(source, usec, source->userdata);
    //             event_lock(event);
    //         }
    //         if (source->enabled == SD_EVENT_ONESHOT) {
    //             source->enabled = SD_EVENT_OFF;
    //         }
    //         break;

    //     case SOURCE_SIGNAL: {
    //         int signaled = 0, result = 0;
    //         k_poll_signal_check(&source->data.signal.signal, &signaled, &result);
    //         if (signaled && source->data.signal.handler) {
    //             // struct signalfd_siginfo si;
    //             // si.ssi_signo = source->data.signal.sig;
    //             // event_unlock(event);
    //             // source->data.signal.handler(source, &si, source->userdata);
    //             // event_lock(event);
    //         }
    //         k_poll_signal_reset(&source->data.signal.signal);
    //         if (source->enabled == SD_EVENT_ONESHOT) {
    //             source->enabled = SD_EVENT_OFF;
    //         }
    //         break;
    //     }

    //     case SOURCE_DEFER:
    //         /* Defer sources use work queue, handled separately */
    //         break;

    //     case SOURCE_POST:
    //         /* Post sources use work queue, handled separately */
    //         break;

    //     default:
    //         break;
    //     }
    // }

    // event->prepared = false;
    // event_unlock(event);

    return 0;
}

int sd_event_run(sd_event *event, uint64_t usec)
{
    // int ret;

    // ret = sd_event_prepare(event);
    // if (ret < 0) {
    //     return ret;
    // }

    // ret = sd_event_wait(event, usec);
    // if (ret < 0) {
    //     return ret;
    // }

    return sd_event_dispatch(event);
}

int sd_event_loop(sd_event *event)
{
    if (!event) {
        return -EINVAL;
    }

    while (1) {
        event_lock(event);
        bool should_exit = event->exit_requested;
        event_unlock(event);

        if (should_exit) {
            break;
        }

        int ret = sd_event_run(event, (uint64_t)-1);
        if (ret < 0 && ret != -EINTR) {
            return ret;
        }
    }

    return 0;
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
