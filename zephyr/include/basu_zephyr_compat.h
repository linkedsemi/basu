#ifndef BASU_ZEPHYR_COMPAT_H
#define BASU_ZEPHYR_COMPAT_H

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>

/* Avoid pthread macro redefinition between Zephyr and newlib */
#ifdef PTHREAD_MUTEX_DEFAULT
#undef PTHREAD_MUTEX_DEFAULT
#endif
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL

/* Define missing socket constants for Zephyr */
#ifdef __ZEPHYR__
#ifndef SO_PASSCRED
#define SO_PASSCRED 16
#endif
#ifndef SCM_RIGHTS
#define SCM_RIGHTS 0x01
#endif
#endif

#include <zephyr/sys/util.h>
#include <stdlib.h>

/* Avoid redefining Zephyr's time macros */
#ifndef USEC_PER_SEC
#define USEC_PER_SEC  ((usec_t) 1000000ULL)
#endif

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC ((usec_t) 1000ULL)
#endif

#ifndef NSEC_PER_USEC
#define NSEC_PER_USEC ((nsec_t) 1000ULL)
#endif

/* Define usec_t and nsec_t if not already defined */
#ifndef usec_t
#define usec_t uint64_t
#endif

#ifndef nsec_t
#define nsec_t uint64_t
#endif

#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC F_DUPFD
#endif

#ifndef FD_CLOEXEC
#define FD_CLOEXEC F_DUPFD
#endif

#ifndef F_GETFD
#define F_GETFD 1
#endif

#ifndef F_SETFD
#define F_SETFD 2
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


#ifndef major
#define major(dev)   (int)(((unsigned long)(dev) >> 20) & 0xfff)
#endif

#ifndef minor
#define minor(dev)   (int)((unsigned long)(dev) & 0xfffff)
#endif

#ifndef makedev
#define makedev(maj, min) \
        ((unsigned long)(((unsigned long)(maj) & 0xfff) << 20) | \
         ((unsigned long)(min) & 0xfffff))
#endif

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

#ifndef F_OK
#define F_OK 0
#endif

// 移除与Zephyr冲突的LOG_*宏定义
#ifndef LOG_PRIMASK
#define LOG_PRIMASK 0x07
#endif
#ifndef LOG_FACMASK
#define LOG_FACMASK 0x03f8
#endif
#ifndef LOG_PRI
#define LOG_PRI(p) ((p) & LOG_PRIMASK)
#endif

// 只有在Zephyr没有定义的情况下才定义这些宏
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif


#ifdef __ZEPHYR__

/* Ensure basic types are defined */
#ifndef pid_t
#define pid_t int32_t
#endif

#ifndef uid_t  
#define uid_t uint32_t
#endif

#ifndef gid_t
#define gid_t uint32_t
#endif

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
ssize_t basu_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
#ifndef readlinkat
#define readlinkat basu_readlinkat
#endif

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
#endif

/* Type definitions for Zephyr */
#ifdef __ZEPHYR__
typedef unsigned long nfds_t;
#endif

/* writev function declaration for Zephyr */
#if defined(__ZEPHYR__)
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
#endif

#endif /* __ZEPHYR__ */

#endif /* BASU_ZEPHYR_COMPAT_H */