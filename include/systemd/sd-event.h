/*
 * Zephyr RTOS based implementation of systemd sd-event API
 *
 * This is a compatibility layer that maps systemd's sd-event API
 * to Zephyr RTOS primitives:
 * - Event loop -> Zephyr k_poll/k_work
 * - Timers -> Zephyr k_timer
 * - IO events -> Zephyr k_poll with sockets/pipes
 * - Signals -> Zephyr k_poll_signal
 * - Child processes -> Zephyr k_thread (limited support)
 */

#ifndef SD_EVENT_H
#define SD_EVENT_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/dlist.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <zephyr/types.h>
#include <time.h>

// Define struct signalfd_siginfo if not available
#ifndef HAVE_SIGNALFD_SIGINFO
struct signalfd_siginfo {
    uint32_t ssi_signo;
    int32_t ssi_errno;
    int32_t ssi_code;
    uint32_t ssi_pid;
    uint32_t ssi_uid;
    int32_t ssi_fd;
    uint32_t ssi_tid;
    uint32_t ssi_band;
    uint32_t ssi_overrun;
    uint32_t ssi_trapno;
    int32_t ssi_status;
    int32_t ssi_int;
    uint64_t ssi_ptr;
    uint64_t ssi_utime;
    uint64_t ssi_stime;
    uint64_t ssi_addr;
    uint16_t ssi_addr_lsb;
    uint8_t pad[46];
};
#define HAVE_SIGNALFD_SIGINFO 1
#endif

#ifdef __ZEPHYR__
#include <zephyr/net/socket.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/timeutil.h>
#endif

// Add missing type definitions
#ifndef pid_t
typedef int pid_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque types */
typedef struct sd_event sd_event;
typedef struct sd_event_source sd_event_source;
typedef struct DispatchContext DispatchContext;  /* Forward declaration for dispatch context */

enum {
        SD_EVENT_INITIAL,
        SD_EVENT_ARMED,
        SD_EVENT_PENDING,
        SD_EVENT_RUNNING,
        SD_EVENT_EXITING,
        SD_EVENT_FINISHED,
        SD_EVENT_PREPARING
};

enum {
        /* And everything in-between and outside is good too */
        SD_EVENT_PRIORITY_IMPORTANT = -100,
        SD_EVENT_PRIORITY_NORMAL = 0,
        SD_EVENT_PRIORITY_IDLE = 100
};

#ifndef EPOLLIN
#define EPOLLIN 0x001
#endif

#ifndef CLOCK_BOOTTIME
#define CLOCK_BOOTTIME          7
#endif

#ifndef CLOCK_REALTIME_ALARM
#define CLOCK_REALTIME_ALARM    8
#endif

#ifndef CLOCK_BOOTTIME_ALARM
#define CLOCK_BOOTTIME_ALARM    9
#endif

/* Handler function types */
typedef int (*sd_event_io_handler_t)(sd_event_source *s, int fd,
                                      uint32_t revents, void *userdata);
typedef int (*sd_event_time_handler_t)(sd_event_source *s, uint64_t usec,
                                        void *userdata);
typedef int (*sd_event_signal_handler_t)(sd_event_source *s,
                                          const struct signalfd_siginfo *si,
                                          void *userdata);
typedef int (*sd_event_child_handler_t)(sd_event_source *s,
                                         const siginfo_t *si,
                                         void *userdata);
typedef int (*sd_event_handler_t)(sd_event_source *s, void *userdata);
typedef void (*sd_event_destroy_t)(void *userdata);

/* Event source types */
enum sd_event_source_type {
    SOURCE_IO,
    SOURCE_TIME,
    SOURCE_SIGNAL,
    SOURCE_CHILD,
    SOURCE_DEFER,
    SOURCE_POST,
    SOURCE_EXIT,
};

/* Enabled states */
enum sd_event_enabled {
    SD_EVENT_OFF = 0,
    SD_EVENT_ON = 1,
    SD_EVENT_ONESHOT = 2,
};

/* Event loop states */
enum sd_event_state {
    SD_EVENT_STATE_PASSIVE = 0,
    SD_EVENT_STATE_RUNNING = 1,
    SD_EVENT_STATE_PREPARING = 2,
    SD_EVENT_STATE_ARMED = 3,
    SD_EVENT_STATE_EXITING = 4,
};

/* IO events (matching POSIX poll) */
#define SD_EVENT_READABLE  0x001
#define SD_EVENT_WRITABLE  0x002
#define SD_EVENT_ERROR     0x004
#define SD_EVENT_HANGUP    0x008

/* Clock IDs */
#define SD_EVENT_CLOCK_REALTIME     0
#define SD_EVENT_CLOCK_MONOTONIC    1
#define SD_EVENT_CLOCK_BOOTTIME     2
#define SD_EVENT_CLOCK_REALTIME_ALARM  3
#define SD_EVENT_CLOCK_BOOTTIME_ALARM  4

/* ============================================================
 * Event Loop Management
 * ============================================================ */

/**
 * @brief Create a new event loop
 * @param event Pointer to store the created event loop
 * @return 0 on success, negative errno on error
 */
int sd_event_new(sd_event **event);

/**
 * @brief Get the default event loop (creates if needed)
 * @param event Pointer to store the default event loop
 * @return 0 on success, negative errno on error
 */
int sd_event_default(sd_event **event);

/**
 * @brief Increase reference count of event loop
 * @param event Event loop
 * @return The event loop
 */
sd_event *sd_event_ref(sd_event *event);

/**
 * @brief Decrease reference count of event loop
 * @param event Event loop
 * @return NULL (always)
 */
sd_event *sd_event_unref(sd_event *event);

/**
 * @brief Prepare event loop for iteration
 * @param event Event loop
 * @return 0 on success, negative errno on error
 */
int sd_event_prepare(sd_event *event);

/**
 * @brief Wait for events
 * @param event Event loop
 * @param usec Timeout in microseconds (usec), (uint64_t)-1 for infinity
 * @return 0 on success, negative errno on error
 */
int sd_event_wait(sd_event *event, uint64_t usec);

/**
 * @brief Dispatch pending events
 * @param event Event loop
 * @return 0 on success, negative errno on error
 */
int sd_event_dispatch(sd_event *event);

/**
 * @brief Run single iteration of event loop
 * @param event Event loop
 * @param usec Timeout in microseconds
 * @return 0 on success, negative errno on error
 */
int sd_event_run(sd_event *event, uint64_t usec);

/**
 * @brief Run event loop continuously
 * @param event Event loop
 * @return 0 on success, negative errno on error
 */
int sd_event_loop(sd_event *event);

/**
 * @brief Exit event loop
 * @param event Event loop
 * @param code Exit code
 * @return 0 on success, negative errno on error
 */
int sd_event_exit(sd_event *event, int code);

/**
 * @brief Get current time
 * @param event Event loop
 * @param clock Clock ID
 * @param usec Pointer to store time in microseconds
 * @return 0 on success, negative errno on error
 */
int sd_event_now(sd_event *event, int clock, uint64_t *usec);

int sd_event_get_fd(sd_event *event);
int sd_event_get_state(sd_event *event);
/**
 * @brief Get exit code
 * @param event Event loop
 * @param code Pointer to store exit code
 * @return 0 on success, negative errno on error
 */
int sd_event_get_exit_code(sd_event *event, int *code);

/**
 * @brief Get watchdog state
 * @param event Event loop
 * @return 1 if watchdog enabled, 0 if disabled, negative errno on error
 */
int sd_event_get_watchdog(sd_event *event);

/**
 * @brief Set watchdog state
 * @param event Event loop
 * @param b Enable/disable watchdog
 * @return 0 on success, negative errno on error
 */
int sd_event_set_watchdog(sd_event *event, int b);

/**
 * @brief Get the file descriptor associated with the event loop
 * @param event Event loop
 * @return File descriptor on success, negative errno on error
 */
int sd_event_get_fd(sd_event *event);

/**
 * @brief Get the current state of the event loop
 * @param event Event loop
 * @return Current state (SD_EVENT_STATE_PASSIVE/ARMED/PREPARING/RUNNING/EXITING)
 */
int sd_event_get_state(sd_event *event);

/**
 * @brief Set the dispatch context for the event loop
 * @param event Event loop
 * @param dispatch External dispatch context to use (NULL to use internal)
 * @return 0 on success, negative errno on error
 * 
 * This allows sharing a dispatch context with other components (e.g., dbus-broker)
 * to avoid conflicts and improve performance.
 */
int sd_event_set_dispatch_context(sd_event *event, DispatchContext *dispatch);

/* ============================================================
 * Event Source Creation
 * ============================================================ */

/**
 * @brief Add IO event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param fd File descriptor
 * @param events Events to watch (SD_EVENT_READABLE, etc.)
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_io(sd_event *event, sd_event_source **source,
                    int fd, uint32_t events,
                    sd_event_io_handler_t callback, void *userdata);

/**
 * @brief Add timer event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param clock Clock ID
 * @param usec Trigger time
 * @param accuracy Accuracy in microseconds
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_time(sd_event *event, sd_event_source **source,
                      int clock, uint64_t usec, uint64_t accuracy,
                      sd_event_time_handler_t callback, void *userdata);
/**
 * @brief Add timer event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param clock Clock ID
 * @param usec Trigger time
 * @param accuracy Accuracy in microseconds
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_time_relative(sd_event *event, sd_event_source **source,
                               int clock, uint64_t usec, uint64_t accuracy,
                               sd_event_time_handler_t callback, void *userdata);
/**
 * @brief Add signal event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param sig Signal number
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_signal(sd_event *event, sd_event_source **source,
                        int sig, sd_event_signal_handler_t callback,
                        void *userdata);

/**
 * @brief Add child process event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param pid Process ID
 * @param options Wait options
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_child(sd_event *event, sd_event_source **source,
                       pid_t pid, int options,
                       sd_event_child_handler_t callback, void *userdata);

/**
 * @brief Add deferred event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_defer(sd_event *event, sd_event_source **source,
                       sd_event_handler_t callback, void *userdata);

/**
 * @brief Add post event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_post(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata);

/**
 * @brief Add exit event source
 * @param event Event loop
 * @param source Pointer to store source
 * @param callback Callback function
 * @param userdata User data
 * @return 0 on success, negative errno on error
 */
int sd_event_add_exit(sd_event *event, sd_event_source **source,
                      sd_event_handler_t callback, void *userdata);

/* ============================================================
 * Event Source Management
 * ============================================================ */

/**
 * @brief Increase reference count of source
 * @param source Event source
 * @return The source
 */
sd_event_source *sd_event_source_ref(sd_event_source *source);

/**
 * @brief Decrease reference count of source
 * @param source Event source
 * @return NULL (always)
 */
sd_event_source *sd_event_source_unref(sd_event_source *source);

/**
 * @brief Get user data
 * @param source Event source
 * @return User data pointer
 */
void *sd_event_source_get_userdata(sd_event_source *source);

/**
 * @brief Set user data
 * @param source Event source
 * @param userdata User data
 * @return Previous user data
 */
void *sd_event_source_set_userdata(sd_event_source *source, void *userdata);

/**
 * @brief Get description
 * @param source Event source
 * @param description Pointer to store description
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_description(sd_event_source *source,
                                     const char **description);

/**
 * @brief Set description
 * @param source Event source
 * @param description Description string
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_description(sd_event_source *source,
                                     const char *description);

/**
 * @brief Set prepare callback
 * @param source Event source
 * @param callback Prepare callback
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_prepare(sd_event_source *source,
                                 sd_event_handler_t callback);

/**
 * @brief Get pending state
 * @param source Event source
 * @return 1 if pending, 0 if not, negative errno on error
 */
int sd_event_source_get_pending(sd_event_source *source);

/**
 * @brief Get priority
 * @param source Event source
 * @param priority Pointer to store priority
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_priority(sd_event_source *source, int64_t *priority);

/**
 * @brief Set priority
 * @param source Event source
 * @param priority Priority value
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_priority(sd_event_source *source, int64_t priority);

/**
 * @brief Get enabled state
 * @param source Event source
 * @param enabled Pointer to store enabled state
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_enabled(sd_event_source *source, int *enabled);

/**
 * @brief Set enabled state
 * @param source Event source
 * @param enabled Enabled state (SD_EVENT_OFF, SD_EVENT_ON, SD_EVENT_ONESHOT)
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_enabled(sd_event_source *source, int enabled);

/**
 * @brief Get floating state
 * @param source Event source
 * @return 1 if floating, 0 if not, negative errno on error
 */
int sd_event_source_get_floating(sd_event_source *source);

/**
 * @brief Set floating state
 * @param source Event source
 * @param b Floating state
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_floating(sd_event_source *source, int b);

/**
 * @brief Set destroy callback
 * @param source Event source
 * @param callback Destroy callback
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_destroy_callback(sd_event_source *source,
                                          sd_event_destroy_t callback);

/**
 * @brief Get destroy callback
 * @param source Event source
 * @param callback Pointer to store callback
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_destroy_callback(sd_event_source *source,
                                          sd_event_destroy_t *callback);

/**
 * @brief Get event from source
 * @param source Event source
 * @return Event loop pointer
 */
sd_event* sd_event_source_get_event(sd_event_source *source);

/* ============================================================
 * IO Source Specific
 * ============================================================ */

/**
 * @brief Get IO file descriptor
 * @param source Event source
 * @return File descriptor, negative errno on error
 */
int sd_event_source_get_io_fd(sd_event_source *source);

/**
 * @brief Set IO file descriptor
 * @param source Event source
 * @param fd File descriptor
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_io_fd(sd_event_source *source, int fd);

/**
 * @brief Get IO events
 * @param source Event source
 * @param events Pointer to store events
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_io_events(sd_event_source *source, uint32_t *events);

/**
 * @brief Set IO events
 * @param source Event source
 * @param events Events to watch
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_io_events(sd_event_source *source, uint32_t events);

/**
 * @brief Get IO returned events
 * @param source Event source
 * @param revents Pointer to store returned events
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_io_revents(sd_event_source *source, uint32_t *revents);

/* ============================================================
 * Time Source Specific
 * ============================================================ */

/**
 * @brief Get time
 * @param source Event source
 * @param usec Pointer to store time
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_time(sd_event_source *source, uint64_t *usec);

/**
 * @brief Set time
 * @param source Event source
 * @param usec Time in microseconds
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_time(sd_event_source *source, uint64_t usec);

/**
 * @brief Get time accuracy
 * @param source Event source
 * @param usec Pointer to store accuracy
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_time_accuracy(sd_event_source *source, uint64_t *usec);

/**
 * @brief Set time accuracy
 * @param source Event source
 * @param usec Accuracy in microseconds
 * @return 0 on success, negative errno on error
 */
int sd_event_source_set_time_accuracy(sd_event_source *source, uint64_t usec);

/* ============================================================
 * Signal Source Specific
 * ============================================================ */

/**
 * @brief Get signal number
 * @param source Event source
 * @return Signal number, negative errno on error
 */
int sd_event_source_get_signal(sd_event_source *source);

/* ============================================================
 * Child Source Specific
 * ============================================================ */

/**
 * @brief Get child PID
 * @param source Event source
 * @param pid Pointer to store PID
 * @return 0 on success, negative errno on error
 */
int sd_event_source_get_child_pid(sd_event_source *source, pid_t *pid);

#ifdef __cplusplus
}
#endif

#endif /* SD_EVENT_H */