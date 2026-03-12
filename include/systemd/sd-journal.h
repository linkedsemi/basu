#ifndef SD_JOURNAL_H_
#define SD_JOURNAL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/posix/syslog.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <stdarg.h>
int     isatty (int __fildes);
int sd_journal_print(int priority, const char *format, ...);
int sd_journal_printv(int priority, const char *format, va_list ap);
int sd_journal_send(const char *format, ...);
int sd_journal_sendv(const struct iovec *iov, int n);
int sd_journal_perror(const char *message);
#ifdef __cplusplus
}
#endif
#endif
