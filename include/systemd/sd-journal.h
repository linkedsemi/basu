#ifndef SD_JOURNAL_H_
#define SD_JOURNAL_H_

#include <zephyr/posix/syslog.h>
#include <zephyr/net/net_ip.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "sd-id128.h"

#ifdef __cplusplus
extern "C" {
#endif

int isatty(int __fildes);
int sd_journal_print(int priority, const char *format, ...);
int sd_journal_printv(int priority, const char *format, va_list ap);
int sd_journal_send(const char *format, ...);
int sd_journal_sendv(const struct iovec *iov, int n);
int sd_journal_perror(const char *message);

typedef struct sd_journal sd_journal;

/* Open flags; subset kept in sync with systemd's sd-journal.h. */
enum {
    SD_JOURNAL_LOCAL_ONLY = 1 << 0,
    SD_JOURNAL_RUNTIME_ONLY = 1 << 1,
    SD_JOURNAL_SYSTEM = 1 << 2,
    SD_JOURNAL_CURRENT_USER = 1 << 3,
};

int sd_journal_open(sd_journal **ret, int flags);
void sd_journal_close(sd_journal *j);

int sd_journal_next(sd_journal *j);
int sd_journal_previous(sd_journal *j);
int sd_journal_next_skip(sd_journal *j, uint64_t skip);

int sd_journal_get_data(sd_journal *j, const char *field, const void **data,
                        size_t *l);
int sd_journal_get_realtime_usec(sd_journal *j, uint64_t *ret);
int sd_journal_get_seqnum(sd_journal *j, uint64_t *ret_seqnum,
                          sd_id128_t *ret_seqnum_id);

int sd_journal_seek_head(sd_journal *j);
int sd_journal_seek_tail(sd_journal *j);
int sd_journal_seek_cursor(sd_journal *j, const char *cursor);

int sd_journal_get_cursor(sd_journal *j, char **ret);
int sd_journal_test_cursor(sd_journal *j, const char *cursor);

#ifdef __cplusplus
}
#endif
#endif
