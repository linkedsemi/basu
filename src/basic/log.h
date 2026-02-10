/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

/* Always define our own logging constants to avoid conflicts */
// Undefine any existing LOG_* macros to avoid conflicts
#ifdef LOG_EMERG
#undef LOG_EMERG
#endif
#ifdef LOG_ALERT
#undef LOG_ALERT
#endif
#ifdef LOG_CRIT
#undef LOG_CRIT
#endif
#ifdef LOG_ERR
#undef LOG_ERR
#endif
#ifdef LOG_WARNING
#undef LOG_WARNING
#endif
#ifdef LOG_NOTICE
#undef LOG_NOTICE
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif

// Define all logging level constants
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
// #define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

// Only define LOG_PRI if not already defined
#ifndef LOG_PRI
#define LOG_PRI(l)  (l)
#endif

#include "macro.h"

/* Define LogRealm as macros instead of enum for Zephyr compatibility */
#define LOG_REALM_SYSTEMD 0
#define LOG_REALM_UDEV 1
#define _LOG_REALM_MAX 2

/* Define LogRealm as integer type */
typedef int LogRealm;

#ifndef LOG_REALM
#  define LOG_REALM LOG_REALM_SYSTEMD
#endif

#define LOG_REALM_PLUS_LEVEL(realm, level)      \
        ((realm) << 10 | (level))
#define LOG_REALM_REMOVE_LEVEL(realm_level)     \
        ((realm_level >> 10))

void log_set_max_level_realm(LogRealm realm, int level);
#define log_set_max_level(level)                \
        log_set_max_level_realm(LOG_REALM, (level))

int log_get_max_level_realm(LogRealm realm) _pure_;

void log_parse_environment_realm(LogRealm realm);
#define log_parse_environment() \
        log_parse_environment_realm(LOG_REALM)

int log_internal_realm(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format, ...) _printf_(6,7);

int log_oom_internal(
                LogRealm realm,
                const char *file,
                int line,
                const char *func);

/* Logging for various assertions */
_noreturn_ void log_assert_failed_realm(
                LogRealm realm,
                const char *text,
                const char *file,
                int line,
                const char *func);
#define log_assert_failed(text, ...) \
        log_assert_failed_realm(LOG_REALM, (text), __VA_ARGS__)

_noreturn_ void log_assert_failed_unreachable_realm(
                LogRealm realm,
                const char *text,
                const char *file,
                int line,
                const char *func);
#define log_assert_failed_unreachable(text, ...) \
        log_assert_failed_unreachable_realm(LOG_REALM, (text), __VA_ARGS__)

void log_assert_failed_return_realm(
                LogRealm realm,
                const char *text,
                const char *file,
                int line,
                const char *func);
#define log_assert_failed_return(text, ...) \
        log_assert_failed_return_realm(LOG_REALM, (text), __VA_ARGS__)

/* Logging with level */
#define log_full_errno_realm(realm, level, error, ...)                  \
        ({                                                              \
                int _level = (level), _e = (error), _realm = (realm);   \
                (log_get_max_level_realm(_realm) >= LOG_PRI(_level))   \
                        ? log_internal_realm(LOG_REALM_PLUS_LEVEL(_realm, _level), _e, \
                                             __FILE__, __LINE__, __func__, __VA_ARGS__) \
                        : -abs(_e);                                     \
        })

#define log_full_errno(level, error, ...)                               \
        log_full_errno_realm(LOG_REALM, (level), (error), __VA_ARGS__)

#define log_full(level, ...) log_full_errno((level), 0, __VA_ARGS__)

/* Normal logging */
#define log_debug(...)     log_full(LOG_DEBUG,   __VA_ARGS__)
#define log_info(...)      log_full(LOG_INFO,    __VA_ARGS__)
#define log_notice(...)    log_full(LOG_NOTICE,  __VA_ARGS__)
#define log_warning(...)   log_full(LOG_WARNING, __VA_ARGS__)
#define log_error(...)     log_full(LOG_ERR,     __VA_ARGS__)

/* Logging triggered by an errno-like error */
#define log_debug_errno(error, ...)     log_full_errno(LOG_DEBUG,   error, __VA_ARGS__)
#define log_notice_errno(error, ...)    log_full_errno(LOG_NOTICE,  error, __VA_ARGS__)
#define log_error_errno(error, ...)     log_full_errno(LOG_ERR,     error, __VA_ARGS__)

#define log_oom() log_oom_internal(LOG_REALM, __FILE__, __LINE__, __func__)