/* Must include Zephyr headers first to avoid type conflicts */
#include <zephyr/sys/util.h>
#include <zephyr/posix/unistd.h>

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/basu_zephyr_compat.h"

/*
 * Minimal, portable implementations for basu on Zephyr/newlib.
 * - Prefer toolchain (newlib) behavior when available
 * - Avoid pulling Zephyr headers here to keep dependencies clean
 */

ssize_t basu_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
        if (!pathname || !buf) {
                errno = EINVAL;
                return -1;
        }

        /* No readlink implementation available in Zephyr
         * Report not supported instead of pretending success. */
        errno = ENOTSUP;
        return -1;
}

int basu_access_fallback(const char *pathname, int mode) {
        if (!pathname) {
                errno = EFAULT;
                return -1;
        }

        /* Provide F_OK semantics via stat(). For R_OK/W_OK/X_OK, report ENOTSUP. */
        if (mode == F_OK) {
            struct stat st;
            return stat(pathname, &st);
        }

        errno = ENOTSUP;
        return -1;
}

/* newlib typically exposes a low-level _isatty; call it when present */
extern int _isatty(int fd);

int basu_isatty(int fd) {
        /* If toolchain provides _isatty, use it. Otherwise return ENOTTY. */
        if (fd < 0) {
                errno = EBADF;
                return 0;
        }

        /* Call into newlib; if it is not linked, the linker will error, which
         * indicates the platform truly lacks TTY support. */
        int r = _isatty(fd);
        if (r == 0) {
                /* Ensure errno conveys not-a-tty when not set */
                if (errno == 0) {
                        errno = ENOTTY;
                }
        }
        return r;
}

#ifdef __ZEPHYR__
// ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
// {
//     ssize_t total = 0;
//     int i;
    
//     for (i = 0; i < iovcnt; i++) {
//         ssize_t written = write(fd, iov[i].iov_base, iov[i].iov_len);
//         if (written < 0) {
//             return -1;
//         }
//         total += written;
//     }
    
//     return total;
// }

// int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask)
// {
//     (void)sigmask;
    
//     /* Convert timespec to milliseconds for poll */
//     int timeout_ms = -1; /* Infinite timeout */
//     if (timeout) {
//         timeout_ms = (timeout->tv_sec * 1000) + (timeout->tv_nsec / 1000000);
//         if (timeout->tv_nsec % 1000000 >= 500000) {
//             timeout_ms++; /* Round up */
//         }
//     }
    
//     return poll(fds, nfds, timeout_ms);
// }
#endif /* __ZEPHYR__ */

/* Missing credential functions for basu */
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/* Define the sd_bus_creds structure locally to avoid include issues */
struct sd_bus_creds;
typedef struct sd_bus_creds sd_bus_creds;

struct sd_bus_creds {
        bool allocated;
        unsigned n_ref;

        uint64_t mask;
        uint64_t augmented;

        uid_t uid;
        uid_t euid;
        uid_t suid;
        uid_t fsuid;
        gid_t gid;
        gid_t egid;
        gid_t sgid;
        gid_t fsgid;

        gid_t *supplementary_gids;
        unsigned n_supplementary_gids;

        pid_t ppid;
        pid_t pid;
        pid_t tid;

        char *comm;
        char *tid_comm;
        char *exe;

        char *cmdline;
        size_t cmdline_size;
        char **cmdline_array;

        char *cgroup;
        char *session;
        char *unit;
        char *user_unit;
        char *slice;
        char *user_slice;

        char *tty;

        uint32_t *capability;

        uint32_t audit_session_id;
        uid_t audit_login_uid;

        char *label;

        char *unique_name;

        char **well_known_names;
        bool well_known_names_driver:1;
        bool well_known_names_local:1;

        char *cgroup_root;

        char *description, *unescaped_description;
};

// sd_bus_creds* bus_creds_new(void) {
//     sd_bus_creds *c = malloc(sizeof(sd_bus_creds));
//     if (!c) return NULL;
//     memset(c, 0, sizeof(*c));
//     c->allocated = true;
//     c->n_ref = 1;
//     return c;
// }

// void bus_creds_done(sd_bus_creds *c) {
//     if (!c) return;
//     /* Clean up any allocated fields - stub for Zephyr */
// }

// int bus_creds_add_more(sd_bus_creds *c, uint64_t mask, pid_t pid, pid_t tid) {
//     if (!c) return -EINVAL;
//     c->mask |= mask;
//     (void)pid;
//     (void)tid;
//     return 0;
// }

// int bus_creds_extend_by_pid(sd_bus_creds *c, uint64_t mask, sd_bus_creds **ret) {
//     if (!c || !ret) return -EINVAL;
//     *ret = bus_creds_new();
//     if (!*ret) return -ENOMEM;
//     (*ret)->mask = mask;
//     return 0;
// }

// sd_bus_creds* sd_bus_creds_ref(sd_bus_creds *c) {
//     if (!c) return NULL;
//     c->n_ref++;
//     return c;
// }

// sd_bus_creds* sd_bus_creds_unref(sd_bus_creds *c) {
//     if (!c) return NULL;
//     if (--c->n_ref == 0) {
//         bus_creds_done(c);
//         free(c);
//         return NULL;
//     }
//     return c;
// }

// void sd_bus_creds_unrefp(sd_bus_creds **c) {
//     if (!c || !*c) return;
//     *c = sd_bus_creds_unref(*c);
// }

// int sd_bus_creds_get_augmented_mask(sd_bus_creds *c, uint64_t *mask) {
//     if (!c || !mask) return -EINVAL;
//     *mask = c->augmented;
//     return 0;
// }

// int sd_bus_creds_has_effective_cap(sd_bus_creds *c, int capability) {
//     (void)c;
//     (void)capability;
//     return 0; /* No capabilities in Zephyr */
// }

// int sd_bus_creds_get_euid(sd_bus_creds *c, uid_t *uid) {
//     if (!c || !uid) return -EINVAL;
//     *uid = c->euid;
//     return 0;
// }

// int sd_bus_creds_get_uid(sd_bus_creds *c, uid_t *uid) {
//     if (!c || !uid) return -EINVAL;
//     *uid = c->uid;
//     return 0;
// }

void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen) {
    const unsigned char *h = haystack;
    const unsigned char *n = needle;

    if (needlelen == 0)
        return (void *)haystack;
    if (haystacklen < needlelen)
        return NULL;

    for (size_t i = 0; i <= haystacklen - needlelen; i++) {
        if (h[i] == n[0] && memcmp(&h[i], n, needlelen) == 0) {
            return (void *)&h[i];
        }
    }
    return NULL;
}

void *mempcpy(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
    return (unsigned char *)dest + n;
}

/* Zephyr doesn't have multi-process support, so wait/waitpid are stubs */
pid_t wait(int *status) {
    (void)status;
    return -1; /* No child processes */
}

pid_t waitpid(pid_t pid, int *status, int options) {
    (void)pid;
    (void)status;
    (void)options;
    return -1; /* No child processes */
}