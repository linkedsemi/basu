/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <locale.h>

/* Locale-related definitions for Zephyr */
#ifdef __NEWLIB__
/* Newlib already defines locale_t, use its definition */
#else
typedef void* locale_t;
#endif
#include <ctype.h>

/* Implementation of strdup for Zephyr */
char *strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        strcpy(copy, s);
    }
    return copy;
}

/* Implementation of strndup for Zephyr */
char *strndup(const char *s, size_t n) {
    if (!s) {
        return NULL;
    }
    
    size_t len = strnlen(s, n);
    char *copy = malloc(len + 1);
    if (copy) {
        strncpy(copy, s, len);
        copy[len] = '\0';
    }
    return copy;
}

/* Implementation of stpcpy for Zephyr */
char *stpcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0') {
        /* Do nothing */
    }
    return d - 1;
}

/* Implementation of strcasecmp for Zephyr */
int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/* Implementation of strchrnul for Zephyr */
#ifndef strchrnul
#ifdef __NEWLIB__
/* Newlib already defines strchrnul, use its definition */
#else
const char *strchrnul(const char *s, int c) {
    while (*s && *s != (char)c) {
        s++;
    }
    return s;
}
#endif
#endif