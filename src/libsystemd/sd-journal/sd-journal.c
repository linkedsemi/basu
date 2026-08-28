#include "sd-journal.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ZEPHYR__
#include <zephyr/sys/printk.h>

/*
 * Zephyr printk() is a printf-like formatter: a stray '%' in a log payload
 * would be interpreted as a conversion specifier.  Everything is therefore
 * rendered into a buffer first and emitted with a single printk("%s", ...)
 * call, which keeps the payload safe and produces one clean line:
 *
 *   [TAG] message (file:func:line) | KEY=value | KEY=value ...
 */

/* A rendered KEY=value pair collected from sd_journal_send varargs. */
struct journal_kv
{
    char key[40];
    char val[192];
};

/* Map a journal priority (0-7) to a short tag. */
static const char *prio_tag(int prio)
{
    static const char *const pri_tag[] = {
        "EMRG", "ALRT", "CRIT", "ERR ", "WARN",
        "NOTE", "INFO", "DBG ",
    };

    if (prio >= 0 && prio <= 7) {
        return pri_tag[prio];
    }
    return "INFO";
}

/*
 * Append a printf-formatted chunk to buf.
 * NOTE: returns the ABSOLUTE new offset (off + written), so callers must
 * assign it directly ("off = bappendf(...)") and never use "off +=".
 */
static size_t bappendf(char *buf, size_t buf_sz, size_t off, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf + off, buf_sz - off, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return off;
    }
    size_t total = off + (size_t)n;
    return total < buf_sz ? total : buf_sz - 1;
}

/* Append len raw bytes of s to buf (no format interpretation). */
static size_t bappend_raw(char *buf, size_t buf_sz, size_t off,
                          const char *s, size_t len)
{
    if (off >= buf_sz - 1) {
        return off;
    }
    size_t n = len;
    if (n > buf_sz - 1 - off) {
        n = buf_sz - 1 - off;
    }
    memcpy(buf + off, s, n);
    buf[off + n] = '\0';
    return off + n;
}

/* Replace embedded newlines with spaces and trim trailing whitespace. */
static void collapse_nl(char *s)
{
    char *r = s;
    char *w = s;
    char *last = s;

    for (; *r; r++) {
        char c = (*r == '\n' || *r == '\r') ? ' ' : *r;
        *w++ = c;
        if (c != ' ') {
            last = w;
        }
    }
    *w = '\0';
    if (last < w) {
        *last = '\0';
    }
}

/* Return the basename of a path. */
static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash != NULL ? slash + 1 : path;
}

/*
 * Render a printf-style value format into buf, consuming the matching
 * arguments from *ap.  Supports %s/%d/%i/%u/%x/%X/%o/%c/%p/%f/%e/%g with
 * l/ll/z length modifiers.  Width/precision are ignored (values are simply
 * rendered with a fixed conversion) but their '*' args are consumed so the
 * varargs walk in the caller stays aligned.
 */
static void render_format(char *buf, size_t buf_sz, const char *fmt, va_list *ap)
{
    size_t off = 0;
    const char *p = fmt;

    if (buf_sz == 0) {
        return;
    }

    while (*p != '\0') {
        if (*p != '%') {
            if (off + 1 < buf_sz) {
                buf[off++] = *p;
            }
            p++;
            continue;
        }
        if (p[1] == '%') {
            if (off + 1 < buf_sz) {
                buf[off++] = '%';
            }
            p += 2;
            continue;
        }

        /* Skip flags. */
        p++;
        while (*p == '#' || *p == '0' || *p == '-' || *p == ' ' || *p == '+') {
            p++;
        }
        /* Skip width. */
        if (*p == '*') {
            (void)va_arg(*ap, int);
            p++;
        } else {
            while (*p >= '0' && *p <= '9') {
                p++;
            }
        }
        /* Skip precision. */
        if (*p == '.') {
            p++;
            if (*p == '*') {
                (void)va_arg(*ap, int);
                p++;
            } else {
                while (*p >= '0' && *p <= '9') {
                    p++;
                }
            }
        }
        /* Length modifier. */
        int is_ll = 0, is_l = 0, is_z = 0;
        if (p[0] == 'l' && p[1] == 'l') {
            is_ll = 1;
            p += 2;
        } else if (*p == 'l') {
            is_l = 1;
            p++;
        } else if (*p == 'z') {
            is_z = 1;
            p++;
        } else if (*p == 't') {
            p++;
        }

        char tmp[96];
        int n = 0;
        switch (*p) {
        case 's': {
            const char *s = va_arg(*ap, const char *);
            n = snprintf(tmp, sizeof(tmp), "%s", s != NULL ? s : "(null)");
            break;
        }
        case 'd':
        case 'i':
            if (is_ll) {
                n = snprintf(tmp, sizeof(tmp), "%lld", va_arg(*ap, long long));
            } else if (is_l) {
                n = snprintf(tmp, sizeof(tmp), "%ld", va_arg(*ap, long));
            } else if (is_z) {
                n = snprintf(tmp, sizeof(tmp), "%zu", va_arg(*ap, size_t));
            } else {
                n = snprintf(tmp, sizeof(tmp), "%d", va_arg(*ap, int));
            }
            break;
        case 'u':
            if (is_ll) {
                n = snprintf(tmp, sizeof(tmp), "%llu", va_arg(*ap, unsigned long long));
            } else if (is_l) {
                n = snprintf(tmp, sizeof(tmp), "%lu", va_arg(*ap, unsigned long));
            } else if (is_z) {
                n = snprintf(tmp, sizeof(tmp), "%zu", va_arg(*ap, size_t));
            } else {
                n = snprintf(tmp, sizeof(tmp), "%u", va_arg(*ap, unsigned int));
            }
            break;
        case 'x':
            if (is_ll) {
                n = snprintf(tmp, sizeof(tmp), "%llx", va_arg(*ap, unsigned long long));
            } else if (is_l) {
                n = snprintf(tmp, sizeof(tmp), "%lx", va_arg(*ap, unsigned long));
            } else if (is_z) {
                n = snprintf(tmp, sizeof(tmp), "%zx", va_arg(*ap, size_t));
            } else {
                n = snprintf(tmp, sizeof(tmp), "%x", va_arg(*ap, unsigned int));
            }
            break;
        case 'X':
            if (is_ll) {
                n = snprintf(tmp, sizeof(tmp), "%llX", va_arg(*ap, unsigned long long));
            } else if (is_l) {
                n = snprintf(tmp, sizeof(tmp), "%lX", va_arg(*ap, unsigned long));
            } else if (is_z) {
                n = snprintf(tmp, sizeof(tmp), "%zX", va_arg(*ap, size_t));
            } else {
                n = snprintf(tmp, sizeof(tmp), "%X", va_arg(*ap, unsigned int));
            }
            break;
        case 'o':
            if (is_ll) {
                n = snprintf(tmp, sizeof(tmp), "%llo", va_arg(*ap, unsigned long long));
            } else if (is_l) {
                n = snprintf(tmp, sizeof(tmp), "%lo", va_arg(*ap, unsigned long));
            } else if (is_z) {
                n = snprintf(tmp, sizeof(tmp), "%zo", va_arg(*ap, size_t));
            } else {
                n = snprintf(tmp, sizeof(tmp), "%o", va_arg(*ap, unsigned int));
            }
            break;
        case 'c':
            n = snprintf(tmp, sizeof(tmp), "%c", va_arg(*ap, int));
            break;
        case 'f':
        case 'F':
            n = snprintf(tmp, sizeof(tmp), "%f", va_arg(*ap, double));
            break;
        case 'e':
        case 'E':
            n = snprintf(tmp, sizeof(tmp), "%e", va_arg(*ap, double));
            break;
        case 'g':
        case 'G':
            n = snprintf(tmp, sizeof(tmp), "%g", va_arg(*ap, double));
            break;
        case 'p':
            n = snprintf(tmp, sizeof(tmp), "%p", va_arg(*ap, void *));
            break;
        default:
            n = snprintf(tmp, sizeof(tmp), "?");
            break;
        }
        p++; /* past the conversion char */

        if (n < 0) {
            continue;
        }
        size_t tl = (size_t)n;
        if (tl > sizeof(tmp) - 1) {
            tl = sizeof(tmp) - 1;
        }
        if (tl > buf_sz - 1 - off) {
            tl = buf_sz - 1 - off;
        }
        memcpy(buf + off, tmp, tl);
        off += tl;
    }
    buf[off] = '\0';
}

/*
 * Replace every occurrence of "{KEY}" in src with the matching value from
 * kvs, writing the result into dst (at most dst_sz bytes, NUL-terminated).
 */
static void substitute_fields(char *dst, size_t dst_sz, const char *src,
                              struct journal_kv *kvs, int nkv)
{
    for (int i = 0; i < nkv; i++) {
        char brace[48];
        int blen = snprintf(brace, sizeof(brace), "{%s}", kvs[i].key);
        const char *r = src;
        char *w = dst;
        size_t wsize = dst_sz - 1; /* bytes we may still write (excl NUL) */

        /* Rewrite src into dst from the start, substituting {KEY} hits. */
        while (*r != '\0') {
            const char *hit = strstr(r, brace);
            if (hit == NULL) {
                size_t n = strlen(r);
                if (n > wsize) {
                    n = wsize;
                }
                memcpy(w, r, n);
                w += n;
                wsize -= n;
                break;
            }
            /* Copy bytes before the hit. */
            size_t n = (size_t)(hit - r);
            if (n > wsize) {
                n = wsize;
            }
            memcpy(w, r, n);
            w += n;
            wsize -= n;

            /* Copy the replacement value. */
            size_t vl = strlen(kvs[i].val);
            if (vl > wsize) {
                vl = wsize;
            }
            memcpy(w, kvs[i].val, vl);
            w += vl;
            wsize -= vl;

            r = hit + blen;
        }
        *w = '\0';

        src = dst; /* next key works on the updated text */
    }
}

/*
 * sd_journal_send() path (phosphor::logging::log<>):
 *   ("PRIORITY=%d", lvl, "MESSAGE=%s", msg, "TRANSACTION_ID=%llu", id,
 *    "KEY=fmt", args..., NULL)
 * Renders every KEY= pair, substitutes {KEY} placeholders in the message
 * (matching systemd sd_journal_send behaviour) and prints:
 *   [TAG] message | KEY=value | KEY=value ...
 */
int basu_journal_run_send_fields(const char *first, va_list ap)
{
    const char *fmt;
    int prio = 6;
    struct journal_kv kvs[16];
    int nkv = 0;
    static char msg[768];
    static char line[1024];
    size_t off = 0;

    msg[0] = '\0';
    line[0] = '\0';

    fmt = first;
    while (fmt != NULL) {
        char val[256];
        val[0] = '\0';

        if (strncmp(fmt, "MESSAGE=", 8) == 0) {
            render_format(val, sizeof(val), fmt + 8, &ap);
            snprintf(msg, sizeof(msg), "%s", val);
        } else if (strncmp(fmt, "PRIORITY=", 9) == 0) {
            render_format(val, sizeof(val), fmt + 9, &ap);
            char *end = NULL;
            long v = strtol(val, &end, 10);
            if (end != val && v >= 0 && v <= 7) {
                prio = (int)v;
            }
        } else if (nkv < 16) {
            const char *eq = strchr(fmt, '=');
            size_t klen = (eq != NULL) ? (size_t)(eq - fmt) : strlen(fmt);
            if (klen >= sizeof(kvs[nkv].key)) {
                klen = sizeof(kvs[nkv].key) - 1;
            }
            memcpy(kvs[nkv].key, fmt, klen);
            kvs[nkv].key[klen] = '\0';

            if (eq != NULL) {
                render_format(kvs[nkv].val, sizeof(kvs[nkv].val), eq + 1, &ap);
            } else {
                kvs[nkv].val[0] = '\0';
            }
            nkv++;
        }
        fmt = va_arg(ap, const char *);
    }

    /* Substitute {KEY} placeholders in the message (systemd behaviour). */
    if (nkv > 0) {
        static char sub[1024];
        substitute_fields(sub, sizeof(sub), msg, kvs, nkv);
        snprintf(msg, sizeof(msg), "%s", sub);
    }

    collapse_nl(msg);

    for (int i = 0; i < nkv; i++) {
        off = bappendf(line, sizeof(line), off, " | %s=%s",
                       kvs[i].key, kvs[i].val);
    }

    if (msg[0] != '\0') {
        printk("[%s] %s%s\n", prio_tag(prio), msg, line);
    } else {
        printk("[%s]%s\n", prio_tag(prio), line);
    }
    return 0;
}
#else
int basu_journal_run_send_fields(const char *first, va_list ap)
{
    (void)first;
    (void)ap;
    return 0;
}
#endif


int sd_journal_print(int priority, const char *format, ...)
{
#ifdef __ZEPHYR__
    va_list ap;

    (void)priority;
    va_start(ap, format);
    vprintk(format, ap);
    va_end(ap);
    printk("\n");
#else
    (void)priority;
    (void)format;
#endif
    return 0;
}

int sd_journal_printv(int priority, const char *format, va_list ap)
{
#ifdef __ZEPHYR__
    (void)priority;
    vprintk(format, ap);
    printk("\n");
#else
    (void)priority;
    (void)format;
    (void)ap;
#endif
    return 0;
}

int sd_journal_send(const char *format, ...)
{
    va_list ap;
    int r;

    va_start(ap, format);
    r = basu_journal_run_send_fields(format, ap);
    va_end(ap);
    return r;
}

/*
 * sd_journal_sendv() path (lg2): the iovec carries pre-rendered fields:
 *   MESSAGE=, LOG2_FMTMSG=, PRIORITY=, CODE_FILE=, CODE_LINE=,
 *   CODE_FUNC=, MESSAGE_ID= and arbitrary KEY=value pairs.
 * Prints:
 *   [TAG] message (file:func:line) | KEY=value | KEY=value ...
 */
int sd_journal_sendv(const struct iovec *iov, int n)
{
#ifdef __ZEPHYR__
    int prio = 6;
    const char *msg = NULL;
    size_t msg_len = 0;
    char file[64] = "";
    char func[64] = "";
    char line[24] = "";

    if (iov == NULL || n <= 0) {
        return 0;
    }

    for (int i = 0; i < n; i++) {
        const char *s = iov[i].iov_base;
        size_t len = iov[i].iov_len;

        if (s == NULL) {
            continue;
        }
        if (len >= 9 && memcmp(s, "PRIORITY=", 9) == 0) {
            unsigned long v = strtoul(s + 9, NULL, 10);
            if (v <= 7UL) {
                prio = (int)v;
            }
        } else if (len >= 8 && memcmp(s, "MESSAGE=", 8) == 0) {
            msg = s + 8;
            msg_len = len - 8;
        } else if (len >= 10 && memcmp(s, "CODE_FILE=", 10) == 0) {
            size_t l = len - 10;
            if (l >= sizeof(file)) {
                l = sizeof(file) - 1;
            }
            memcpy(file, s + 10, l);
            file[l] = '\0';
        } else if (len >= 10 && memcmp(s, "CODE_LINE=", 10) == 0) {
            size_t l = len - 10;
            if (l >= sizeof(line)) {
                l = sizeof(line) - 1;
            }
            memcpy(line, s + 10, l);
            line[l] = '\0';
        } else if (len >= 10 && memcmp(s, "CODE_FUNC=", 10) == 0) {
            size_t l = len - 10;
            if (l >= sizeof(func)) {
                l = sizeof(func) - 1;
            }
            memcpy(func, s + 10, l);
            func[l] = '\0';
        }
    }

    static char buf[1024];
    size_t off = 0;

    off = bappendf(buf, sizeof(buf), off, "[%s]", prio_tag(prio));
    if (msg != NULL) {
        off = bappendf(buf, sizeof(buf), off, " ");
        off = bappend_raw(buf, sizeof(buf), off, msg, msg_len);
    }
    if (func[0] != '\0' && line[0] != '\0') {
        if (file[0] != '\0') {
            off = bappendf(buf, sizeof(buf), off, " (%s:%s:%s)",
                           base_name(file), func, line);
        } else {
            off = bappendf(buf, sizeof(buf), off, " (%s:%s)", func, line);
        }
    }

    for (int i = 0; i < n; i++) {
        const char *s = iov[i].iov_base;
        size_t len = iov[i].iov_len;

        if (s == NULL || len == 0) {
            continue;
        }
        if (memcmp(s, "PRIORITY=", 9) == 0 || memcmp(s, "MESSAGE=", 8) == 0 ||
            memcmp(s, "CODE_FILE=", 10) == 0 || memcmp(s, "CODE_LINE=", 10) == 0 ||
            memcmp(s, "CODE_FUNC=", 10) == 0 || memcmp(s, "LOG2_FMTMSG=", 12) == 0 ||
            memcmp(s, "MESSAGE_ID=", 11) == 0) {
            continue;
        }
        off = bappendf(buf, sizeof(buf), off, " | ");
        off = bappend_raw(buf, sizeof(buf), off, s, len);
    }

    printk("%s\n", buf);
#else
    (void)iov;
    (void)n;
#endif
    return 0;
}

int sd_journal_perror(const char *message)
{
#ifdef __ZEPHYR__
    if (message != NULL) {
        printk("%s\n", message);
    }
#else
    (void)message;
#endif
    return 0;
}

/*
 * Journal browsing APIs below are stubbed out for Zephyr: there is no
 * persistent journal backend, so every operation fails with -ENOSYS while
 * keeping the systemd-compatible function prototypes.
 */

int sd_journal_open(sd_journal **ret, int flags)
{
    (void)ret;
    (void)flags;
    return -ENOSYS;
}

void sd_journal_close(sd_journal *j)
{
    (void)j;
}

int sd_journal_next(sd_journal *j)
{
    (void)j;
    return -ENOSYS;
}

int sd_journal_previous(sd_journal *j)
{
    (void)j;
    return -ENOSYS;
}

int sd_journal_next_skip(sd_journal *j, uint64_t skip)
{
    (void)j;
    (void)skip;
    return -ENOSYS;
}

int sd_journal_get_data(sd_journal *j, const char *field, const void **data,
                        size_t *l)
{
    (void)j;
    (void)field;
    (void)data;
    (void)l;
    return -ENOSYS;
}

int sd_journal_get_realtime_usec(sd_journal *j, uint64_t *ret)
{
    (void)j;
    (void)ret;
    return -ENOSYS;
}

int sd_journal_get_seqnum(sd_journal *j, uint64_t *ret_seqnum,
                          sd_id128_t *ret_seqnum_id)
{
    (void)j;
    (void)ret_seqnum;
    (void)ret_seqnum_id;
    return -ENOSYS;
}

int sd_journal_seek_head(sd_journal *j)
{
    (void)j;
    return -ENOSYS;
}

int sd_journal_seek_tail(sd_journal *j)
{
    (void)j;
    return -ENOSYS;
}

int sd_journal_seek_cursor(sd_journal *j, const char *cursor)
{
    (void)j;
    (void)cursor;
    return -ENOSYS;
}

int sd_journal_get_cursor(sd_journal *j, char **ret)
{
    (void)j;
    (void)ret;
    return -ENOSYS;
}

int sd_journal_test_cursor(sd_journal *j, const char *cursor)
{
    (void)j;
    (void)cursor;
    return -ENOSYS;
}
