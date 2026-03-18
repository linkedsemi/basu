
/* Define types before including any headers to avoid conflicts */
#ifndef __clock_t_defined
typedef unsigned long clock_t;
#define __clock_t_defined
#endif

#ifndef __clockid_t_defined
typedef unsigned long clockid_t;
#define __clockid_t_defined
#endif

#ifndef __timer_t_defined
typedef unsigned long timer_t;
#define __timer_t_defined
#endif

#ifndef __pid_t_defined
typedef int pid_t;
#define __pid_t_defined
#endif

/* Include Zephyr headers first */
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

/* Suppress cbprintf prototype warnings */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <zephyr/sys/cbprintf.h>
#pragma GCC diagnostic pop
#include <zephyr/sys_clock.h>
#include <zephyr/posix/fcntl.h>
#include <sys/types.h> /* For uid_t type */

/* Fix PTHREAD_MUTEX_DEFAULT redefinition */
#pragma push_macro("PTHREAD_MUTEX_DEFAULT")
#ifdef PTHREAD_MUTEX_DEFAULT
#undef PTHREAD_MUTEX_DEFAULT
#endif

/* Include config.h before basu_zephyr_compat.h to ensure proper configuration */
#include "config.h"

/* Now include basu_zephyr_compat.h to fix SD_BUS_VTABLE_DEPRECATED macro */
#include "basu_zephyr_compat.h"

/* Undefine log macros before including basu's log.h to avoid conflicts */
#ifdef __ZEPHYR__
#undef log_error
#undef log_info
#undef log_debug
#endif


#ifdef __cplusplus
extern "C" {
#endif
int issetugid(void);
// Use uid_t for geteuid and getuid to match the implementation
#include <sys/types.h>
uid_t geteuid(void);
uid_t getuid(void);
//int isatty(int fd);
#ifdef __cplusplus
}
#endif

/* Restore original PTHREAD_MUTEX_DEFAULT definition */
#pragma pop_macro("PTHREAD_MUTEX_DEFAULT")