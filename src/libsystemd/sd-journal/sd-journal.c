#include "sd-journal.h"
int sd_journal_print(int priority, const char *format, ...)
{
    return 0;
}
int sd_journal_printv(int priority, const char *format, va_list ap)
{
    return 0;
}
int sd_journal_send(const char *format, ...)
{
    return 0;
}
int sd_journal_sendv(const struct iovec *iov, int n)
{
    return 0;
}
int sd_journal_perror(const char *message)
{
    return 0;
}