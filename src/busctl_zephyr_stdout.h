/* SPDX-License-Identifier: LGPL-2.1+ */
/*
 * Zephyr stdout redirection for busctl.
 *
 * busctl (and the dump helpers it calls in busctl-introspect.c / bus-dump.c)
 * print with the plain C stdio API. On Zephyr those writes land on the
 * *default* console (the UART), not on the shell backend of the session that
 * ran the command. So when busctl is invoked over the wolfSSH shell backend
 * its output ends up on the serial console instead of the SSH channel.
 *
 * Including this header redirects every stdio call whose destination is
 * `stdout` to the active shell instance (g_busctl_shell), which the busctl
 * shell wrapper sets right before calling basu_busctl_entry. Writes to any
 * other FILE* (notably the open_memstream capture buffer used by
 * format_cmdline) are left untouched, so internal formatting keeps working.
 * When g_busctl_shell is NULL the real console printf is used as a fallback so
 * output is never lost.
 *
 * The helper functions (busctl_printf / busctl_puts / busctl_putchar) are
 * defined once in busctl.c; this header only declares them and provides the
 * macros, so it is safe to include from multiple translation units.
 */
#ifndef BUSCTL_ZEPHYR_STDOUT_H
#define BUSCTL_ZEPHYR_STDOUT_H

#ifdef __ZEPHYR__

#include <zephyr/shell/shell.h>

/* Active shell for the running busctl command; set by the busctl shell
 * wrapper (app/src/busctl_shell.c) before basu_busctl_entry(). */
extern const struct shell *g_busctl_shell;

int busctl_printf(const char *fmt, ...);
int busctl_puts(const char *s);
int busctl_putchar(int c);

/* Redirect stdout-bound stdio to the active shell backend. The (fn) form
 * bypasses the macro to reach the real libc function for non-stdout streams. */
#define printf(...)    busctl_printf(__VA_ARGS__)
#define puts(s)        busctl_puts(s)
#define putchar(c)     busctl_putchar(c)
#define fflush(stream) \
	((stream) == stdout ? 0 : (fflush)(stream))
#define fputs(s, stream) \
	((stream) == stdout ? busctl_puts(s) : (fputs)(s, stream))
#define fputc(c, stream) \
	((stream) == stdout ? busctl_putchar(c) : (fputc)(c, stream))
#define fprintf(stream, ...) \
	((stream) == stdout ? busctl_printf(__VA_ARGS__) : (fprintf)(stream, __VA_ARGS__))

#endif /* __ZEPHYR__ */
#endif /* BUSCTL_ZEPHYR_STDOUT_H */
