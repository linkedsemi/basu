
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

/* Fix PTHREAD_MUTEX_DEFAULT redefinition */
#pragma push_macro("PTHREAD_MUTEX_DEFAULT")
#ifdef PTHREAD_MUTEX_DEFAULT
#undef PTHREAD_MUTEX_DEFAULT
#endif

/* Suppress cbprintf prototype warnings */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <zephyr/sys/cbprintf.h>
#pragma GCC diagnostic pop
#include <zephyr/sys_clock.h>
#include <zephyr/posix/fcntl.h>
#include "config.h"
#include "basu_zephyr_compat.h"



#ifdef __cplusplus
extern "C" {
#endif
int issetugid(void);
int geteuid(void);
int getuid(void);
//int isatty(int fd);
#ifdef __cplusplus
}
#endif

/* Restore original PTHREAD_MUTEX_DEFAULT definition */
#pragma pop_macro("PTHREAD_MUTEX_DEFAULT")
