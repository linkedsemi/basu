#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "../include/basu_zephyr_compat.h"

/* Implementation of vasprintf for Zephyr */
int vasprintf(char **strp, const char *fmt, va_list ap) {
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

void *memrchr(const void *s, int c, size_t n) {
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

void explicit_bzero(void *s, size_t n) {
    memset(s, 0, n);
    /* Compiler barrier to prevent optimization */
    __asm__ volatile("" ::: "memory");
}

/* File locking functions - stubs since Zephyr doesn't support them */
void flockfile(FILE *stream) {
    /* No-op in Zephyr */
    (void)stream; /* Suppress unused parameter warning */
}

void funlockfile(FILE *stream) {
    /* No-op in Zephyr */
    (void)stream; /* Suppress unused parameter warning */
}

/* Only define ferror_unlocked if not already defined by newlib */
#ifdef __NEWLIB__
/* Newlib already defines ferror_unlocked, use its definition */
#else
int ferror_unlocked(FILE *stream) {
    return ferror(stream);
}
#endif

/* Implementation of locale functions */
#ifndef newlocale
locale_t newlocale(int category_mask, const char *locale, locale_t base) {
    (void)category_mask;
    (void)locale;
    (void)base;
    /* Return a non-NULL value to avoid errors */
    return (locale_t)1;
}
#endif

#ifndef freelocale
void freelocale(locale_t locale) {
    (void)locale;
    /* No-op in Zephyr */
}
#endif

uid_t geteuid(void) {
    /* Return 0 (root) as default in Zephyr */
    return 0;
}

uid_t getuid(void) {
    /* Return 0 (root) as default in Zephyr */
    return 0;
}

unsigned int major(dev_t dev) {
    return (unsigned int)((dev >> 8) & 0xfff);
}

unsigned int minor(dev_t dev) {
    return (unsigned int)(dev & 0xff);
}

/* Access fallback implementation */
int basu_access_fallback(const char *pathname, int mode) {
    (void)pathname;
    (void)mode;
    errno = ENOSYS;
    return -1;
}

/* Isatty implementation */
int basu_isatty(int fd) {
    (void)fd;
    errno = ENOSYS;
    return 0;
}

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