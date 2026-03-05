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

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>  /* Required for tolower function */
#include <stdarg.h> /* Required for vasprintf function */

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

#ifdef LOG_ERR
#undef LOG_ERR
#define LOG_ERR 3
#endif

/* Provide log_error stub for Zephyr */
#ifndef log_error
#define log_error(fmt, ...) do { \
    LOG_ERR(fmt ": %s", ##__VA_ARGS__, strerror(errno)); \
} while(0)
#endif

#ifndef log_info
#define log_info(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)
#endif

#ifndef log_debug
#define log_debug(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
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

#ifndef uid_t  
#define uid_t uint32_t
#endif

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
#include <unistd.h>
#include <stdio.h>  /* Add stdio.h for FILE type */
#endif

/* Type definitions for Zephyr */
typedef unsigned long nfds_t;

/* Define missing functions */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
void *mempcpy(void *dest, const void *src, size_t n);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

/* Add missing function declarations */
int asprintf(char **strp, const char *fmt, ...);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
FILE *open_memstream(char **ptr, size_t *sizeloc);

/* Implementations or stubs for missing POSIX functions */
#ifndef issetugid
static inline int issetugid(void) {
    return 0; /* Not applicable in Zephyr */
}
#endif

/* Implementation of vasprintf for Zephyr */
static inline int vasprintf(char **strp, const char *fmt, va_list ap) {
    int len;
    char *str;
    
    /* First try with a reasonable size */
    len = vsnprintf(NULL, 0, fmt, ap);
    if (len < 0) {
        return -1;
    }
    
    str = (char *)malloc(len + 1);
    if (!str) {
        return -1;
    }
    
    len = vsnprintf(str, len + 1, fmt, ap);
    if (len < 0) {
        free(str);
        return -1;
    }
    
    *strp = str;
    return len;
}

static inline char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *dup = (char *)malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

/* Implementation of strndup for Zephyr */
static inline char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

/* Implementation of stpcpy for Zephyr */
static inline char *stpcpy(char *dest, const char *src) {
    while ((*dest++ = *src++) != '\0');
    return --dest;
}

/* Implementation of memrchr for Zephyr */
static inline void *memrchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    const unsigned char *e = p + n;
    
    if (n == 0)
        return NULL;
    
    e--;
    while (e >= p) {
        if (*e == (unsigned char)c)
            return (void *)e;
        e--;
    }
    
    return NULL;
}

/* Implementation of strchrnul for Zephyr */
static inline char *strchrnul(const char *s, int c) {
    if (!s) return NULL;
    
    while (*s != '\0' && *s != (char)c) {
        s++;
    }
    
    return (char *)s;
}

/* Implementation of explicit_bzero for Zephyr */
static inline void explicit_bzero(void *s, size_t n) {
    memset(s, 0, n);
    /* Compiler barrier to prevent optimization */
    __asm__ volatile("" ::: "memory");
}

static inline int fileno(FILE *stream) {
    /* Zephyr doesn't support fileno, return -1 or handle appropriately */
    (void)stream; /* Suppress unused parameter warning */
    errno = ENOSYS;
    return -1;
}

/* File locking functions - stubs since Zephyr doesn't support them */
static inline void flockfile(FILE *stream) {
    /* No-op in Zephyr */
    (void)stream; /* Suppress unused parameter warning */
}

static inline void funlockfile(FILE *stream) {
    /* No-op in Zephyr */
    (void)stream; /* Suppress unused parameter warning */
}

static inline int ferror_unlocked(FILE *stream) {
    return ferror(stream);
}

/* Implementation for readlinkat since Zephyr doesn't provide it */
#ifndef readlinkat
static inline ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    /* Simple stub that always fails - implement properly if needed */
    (void)dirfd;    /* Suppress unused parameter warnings */
    (void)pathname;
    (void)buf;
    (void)bufsiz;
    errno = ENOSYS;
    return -1;
}
#endif

#if 0
/* Implementation of open_memstream for Zephyr */
inline FILE *open_memstream(char **ptr, size_t *sizeloc) {
    FILE *stream;
    char *buffer;
    size_t buffer_size = 4096; /* Initial buffer size */
    
    if (!ptr || !sizeloc) {
        errno = EINVAL;
        return NULL;
    }
    
    /* Allocate initial buffer */
    buffer = malloc(buffer_size);
    if (!buffer) {
        errno = ENOMEM;
        return NULL;
    }
    
    /* Open the buffer as a memory stream */
    stream = fmemopen(buffer, buffer_size, "w");
    if (!stream) {
        free(buffer);
        return NULL;
    }
    
    /* Store the buffer pointer and initial size */
    *ptr = buffer;
    *sizeloc = 0; /* Initial size is 0 */
    
    return stream;
}
#endif

/* Define typeof as __typeof__ for Zephyr compatibility */
#ifndef typeof
#define typeof __typeof__
#endif

/* Locale-related definitions for Zephyr */
#ifndef locale_t
/* Simple typedef for locale_t since Zephyr doesn't provide it */
typedef void* locale_t;
#endif

/* Locale functions stubs */
static inline locale_t newlocale(int category_mask, const char *locale, locale_t base) {
    (void)category_mask;
    (void)locale;
    (void)base;
    errno = ENOSYS;
    return NULL;
}

static inline void freelocale(locale_t locale) {
    (void)locale;
    /* No-op in Zephyr */
}

static inline double strtod_l(const char *nptr, char **endptr, locale_t loc) {
    (void)loc;
    return strtod(nptr, endptr);
}

/* String comparison functions */
static inline int strcasecmp(const char *s1, const char *s2) {
    /* Simple case-insensitive comparison */
    while (*s1 && *s2 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }
    return tolower(*s1) - tolower(*s2);
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    /* Simple case-insensitive comparison with limit */
    while (n > 0 && *s1 && *s2 && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return tolower(*s1) - tolower(*s2);
}

/* Process-related functions */
static inline uid_t geteuid(void) {
    /* Return 0 (root) as default in Zephyr */
    return 0;
}

static inline uid_t getuid(void) {
    /* Return 0 (root) as default in Zephyr */
    return 0;
}

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

/* Device number functions */
static inline unsigned int major(dev_t dev) {
    return (unsigned int)((dev >> 8) & 0xfff);
}

static inline unsigned int minor(dev_t dev) {
    return (unsigned int)(dev & 0xff);
}



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
