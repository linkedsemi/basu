#ifndef BASU_ZEPHYR_COMPAT_H
#define BASU_ZEPHYR_COMPAT_H

#pragma once

/* Fix for SD_BUS_VTABLE_DEPRECATED being defined as empty string */
/* This must come before any includes that use this macro */
#if defined(SD_BUS_VTABLE_DEPRECATED)
  #undef SD_BUS_VTABLE_DEPRECATED
  #define SD_BUS_VTABLE_DEPRECATED (1ULL << 0)
#endif

/* Make sure it's defined even if not previously defined */
#ifndef SD_BUS_VTABLE_DEPRECATED
  #define SD_BUS_VTABLE_DEPRECATED (1ULL << 0)
#endif

/* Define _noreturn_ for Zephyr compatibility */
/* This must be defined before including any basu headers that use it */
/* But only if not already defined elsewhere */
#ifndef _noreturn_
#if __STDC_VERSION__ >= 201112L
#define _noreturn_ _Noreturn
#else
#define _noreturn_ __attribute__((noreturn))
#endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>  /* Required for tolower function */
#include <stdarg.h> /* Required for vasprintf function */
#include <sys/uio.h> /* For struct iovec */
#include <poll.h>    /* For struct pollfd */
#include <zephyr/logging/log.h> /* For Zephyr logging */

/* Locale-related definitions for Zephyr */
#ifndef __locale_t_defined
/* Only define locale_t if not already defined by newlib */
#ifdef __NEWLIB__
/* Newlib already defines locale_t, use its definition */
#else
typedef void* locale_t;
#endif
#define __locale_t_defined
#endif

/* Define __gnuc_va_list for Zephyr compatibility */
#ifndef __gnuc_va_list
#define __gnuc_va_list __va_list
#endif

/* Define SSIZE_MAX if not already defined */
#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)((size_t)-1 / 2))
#endif

#include <sys/types.h>

/* Avoid pthread macro redefinition between Zephyr and newlib */
/* In Zephyr, we use the Zephyr-defined pthread_mutexattr_t and mutex types */
/* If PTHREAD_MUTEX_DEFAULT is already defined by Zephyr or newlib, use that definition */
#ifdef PTHREAD_MUTEX_DEFAULT
/* Already defined, do nothing */
#else
/* Define it using PTHREAD_MUTEX_NORMAL as the default */
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
#endif

/* Define missing socket constants for Zephyr */
#ifdef __ZEPHYR__
#ifndef SO_PASSCRED
#define SO_PASSCRED 16
#endif
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 0x01
#endif

/* File descriptor constants */
#ifndef F_GETFD
#define F_GETFD 1
#endif

#ifndef F_SETFD
#define F_SETFD 2
#endif

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 10
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

#ifndef F_DUPFD
#define F_DUPFD 0
#endif

#endif

#include <zephyr/sys/util.h>
#include <stdlib.h>

/* Use Zephyr's time macros instead of redefining them */

/* Define usec_t and nsec_t if not already defined */
#ifndef usec_t
#define usec_t uint64_t
#endif

#ifndef nsec_t
#define nsec_t uint64_t
#endif

#ifndef F_GETFD
#define F_GETFD 1
#endif

#ifndef F_SETFD
#define F_SETFD 2
#endif

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC 10
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

#ifndef F_DUPFD
#define F_DUPFD 0
#endif

#ifndef MSG_CMSG_CLOEXEC
#define MSG_CMSG_CLOEXEC 0
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX INT_MAX
#endif


/* Zephyr should provide these macros */

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

/* Define missing file descriptor constants */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef F_OK
#define F_OK 0
#endif

/* Use Zephyr's logging macros instead of redefining them */
#ifdef LOG_PRI
#undef LOG_PRI
#endif

#ifndef LOG_ERR
#define LOG_ERR 3
#endif

/* Provide log_error stub for Zephyr only if not already defined by basu */
#ifdef __ZEPHYR__
/* Save Zephyr log macros with different names */
#define ZEPHYR_LOG_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define ZEPHYR_LOG_INF(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)
#define ZEPHYR_LOG_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)

/* Temporarily undefine Zephyr log macros to avoid conflicts */
#undef LOG_ERR
#undef LOG_INFO
#undef LOG_DEBUG

/* Define basu-compatible log level constants */
#define LOG_ERR     3
#define LOG_INFO    6
#define LOG_DEBUG   7
#endif /* __ZEPHYR__ */



/* String utility functions */
#ifndef strdup
char *strdup(const char *s);
#endif

#ifndef strndup
char *strndup(const char *s, size_t n);
#endif

#ifndef stpcpy
char *stpcpy(char *dest, const char *src);
#endif

#ifndef strcasecmp
int strcasecmp(const char *s1, const char *s2);
#endif

#ifndef strtod_l
/* Only declare strtod_l if not already declared by newlib */
#ifdef __NEWLIB__
/* Newlib already defines strtod_l, use its definition */
#else
double strtod_l(const char *nptr, char **endptr, locale_t loc);
#endif
#endif

// 只有在Zephyr没有定义的情况下才定义这些宏
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* File open flags */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif



#ifdef __ZEPHYR__

/* Ensure basic types are defined */
#ifndef pid_t
#define pid_t int32_t
#endif

// #ifndef uid_t  
// #define uid_t uint32_t
// #endif

/* Use Zephyr's definition of gid_t (unsigned short) to avoid conflicts */
#undef gid_t
#define gid_t unsigned short

/* Socket credentials structure */
#ifndef HAVE_STRUCT_UCRED
#define HAVE_STRUCT_UCRED
struct ucred {
    pid_t pid;    /* PID of sending process */
    uid_t uid;    /* UID of sending process */
    gid_t gid;    /* GID of sending process */
};
#endif

/* Socket options for credentials */
#ifndef SO_PEERCRED
#define SO_PEERCRED 17
#endif

#ifndef SO_PEERSEC
#define SO_PEERSEC 31
#endif

/* Provide declarations; implementation in tool/sys_compat.c */
int basu_access_fallback(const char *pathname, int mode);
#ifndef access
#define access basu_access_fallback
#endif


#ifndef BASU_HAVE_ISATTY_PROTO
#define BASU_HAVE_ISATTY_PROTO 1
#ifdef __cplusplus
extern "C" {
#endif
int basu_isatty(int fd);
#ifndef isatty
#define isatty basu_isatty
#endif
#ifdef __cplusplus
}
#endif
#endif


#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* mmap/munmap function declarations for Zephyr */
#if defined(__ZEPHYR__)
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);
#endif

/* IOVEC_MAKE_STRING is defined in io-util.h, so don't redefine it here */

/* Declare write function if not available */
#ifndef write
// #include <unistd.h>
#include <stdio.h>  /* Add stdio.h for FILE type */
#include <signal.h>
#endif

/* Type definitions for Zephyr */
// typedef unsigned long nfds_t;

/* Define missing functions */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
void *mempcpy(void *dest, const void *src, size_t n);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

/* Add missing function declarations */
int asprintf(char **strp, const char *fmt, ...);
int vasprintf(char **strp, const char *fmt, va_list ap);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
FILE *open_memstream(char **ptr, size_t *sizeloc);

/* String utility functions */
char *strdup(const char *s);
char *strndup(const char *s, size_t n);
char *stpcpy(char *dest, const char *src);
int strcasecmp(const char *s1, const char *s2);
/* Only declare strchrnul if not already declared by newlib */
#ifdef __NEWLIB__
/* Newlib already defines strchrnul, use its definition */
#else
const char *strchrnul(const char *s, int c);
#endif

/* Implementations or stubs for missing POSIX functions */
#ifndef issetugid
static int issetugid(void) {
    return 0; /* Not applicable in Zephyr */
}
#endif

/* Only define functions if they haven't been defined by the standard library */

/* Function declarations - implementations in tool/sys_compat.c */
void *memrchr(const void *s, int c, size_t n);
void explicit_bzero(void *s, size_t n);
int fileno(FILE *stream);
void flockfile(FILE *stream);
void funlockfile(FILE *stream);
/* Only declare ferror_unlocked if not already declared by newlib */
#ifdef __NEWLIB__
/* Newlib already defines ferror_unlocked, use its definition */
#else
int ferror_unlocked(FILE *stream);
#endif
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);

/* Define typeof as __typeof__ for Zephyr compatibility */
#ifndef typeof
#define typeof __typeof__
#endif


/* Process-related functions */
#ifndef geteuid
uid_t geteuid(void);
#endif

#ifndef getuid
uid_t getuid(void);
#endif

/* Function declarations for locale and process functions */
/* Only declare newlocale if not already declared by newlib */
#ifdef __NEWLIB__
/* Newlib already defines newlocale, use its definition */
#else
locale_t newlocale(int category_mask, const char *locale, locale_t base);
#endif

/* Only declare freelocale if not already declared by newlib */
#ifdef __NEWLIB__
/* Newlib already defines freelocale, use its definition */
#else
void freelocale(locale_t locale);
#endif
uid_t geteuid(void);
uid_t getuid(void);

/* Ensure LOG_PRI is defined */
#ifndef LOG_PRI
#define LOG_PRI(p) ((p) & LOG_PRIMASK)
#endif

#endif /* End of __ZEPHYR__ block */



/* Ensure LOG_PRIMASK is defined */
#ifndef LOG_PRIMASK
#define LOG_PRIMASK 0x07
#endif

/* Define missing constants */
#define NOBODY_USER_NAME "nobody"

/* Device number functions - declarations */
unsigned int major(dev_t dev);
unsigned int minor(dev_t dev);



/* Ensure SD_BUS_VTABLE_DEPRECATED is properly defined */
/* Handle the case where it might be defined as empty string by cmake */
#ifdef SD_BUS_VTABLE_DEPRECATED
#undef SD_BUS_VTABLE_DEPRECATED
#endif
#define SD_BUS_VTABLE_DEPRECATED (1ULL << 0)

/* Package information */
#ifndef PACKAGE_STRING
#define PACKAGE_STRING "basu"
#endif

#endif /* BASU_ZEPHYR_COMPAT_H */