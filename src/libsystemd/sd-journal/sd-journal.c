#include "sd-journal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __ZEPHYR__
#include <zephyr/sys/printk.h>


static void skip_args_for_format(const char *fmt, va_list *ap)
{
  for (const char *p = fmt; *p; p++) {
    if (*p != '%') {
      continue;
    }
    if (p[1] == '%') {
      p++;
      continue;
    }
    p++;
    

    while (*p == '#' || *p == '0' || *p == '-' || *p == ' ' || *p == '+' || *p == '\'') {
      p++;
    }
    

    if (*p == '*') {
      (void)va_arg(*ap, int);
      p++;
    } else {
      while (*p >= '0' && *p <= '9') {
        p++;
      }
    }
    

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
    }

   switch (*p) {
    case 's':
      (void)va_arg(*ap, const char *);
      break;
    case 'd':
    case 'i':
    case 'c': 
      if (is_ll) (void)va_arg(*ap, long long);
      else if (is_l) (void)va_arg(*ap, long);
      else if (is_z) (void)va_arg(*ap, size_t);
      else (void)va_arg(*ap, int);
      break;
    case 'u':
    case 'x':
    case 'X':
    case 'o': /* 八进制 */
      if (is_ll) (void)va_arg(*ap, unsigned long long);
      else if (is_l) (void)va_arg(*ap, unsigned long);
      else if (is_z) (void)va_arg(*ap, size_t);
      else (void)va_arg(*ap, unsigned int);
      break;
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
      (void)va_arg(*ap, double);
      break;
    case 'p':
      (void)va_arg(*ap, void *);
      break;
    default:
      break;
    }
  }
}

int basu_journal_run_send_fields(const char *first, va_list ap)
{
  const char *fmt;
  int prio = 6;
  const char *msg = NULL;
  va_list ap_copy;

  va_copy(ap_copy, ap);
  fmt = first;
  while (fmt != NULL) {
    if (strncmp(fmt, "MESSAGE=", 8) == 0) {
      msg = va_arg(ap_copy, const char *);
    } else if (strncmp(fmt, "PRIORITY=", 9) == 0) {
      prio = va_arg(ap_copy, int);
    } else {
      skip_args_for_format(fmt, &ap_copy);
    }
    fmt = va_arg(ap_copy, const char *);
  }
  va_end(ap_copy);

  static const char *const pri_tag[] = {
    "EMRG", "ALRT", "CRIT", "ERR ", "WARN",
    "NOTE", "INFO", "DBG ",
  };
  const char *tag = "INFO";

  if (prio >= 0 && prio <= 7) {
    tag = pri_tag[prio];
  }
  
  printk("[log:%s] ", tag);
  if (msg != NULL) {
    printk("MESSAGE=%s | ", msg);
  }


  fmt = first;
  while (fmt != NULL) {
    if (strncmp(fmt, "MESSAGE=", 8) != 0 && strncmp(fmt, "PRIORITY=", 9) != 0) {
      printk("%s | ", fmt); 
    }
    skip_args_for_format(fmt, &ap);
    fmt = va_arg(ap, const char *);
  }
  
  printk("\n");
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

int sd_journal_sendv(const struct iovec *iov, int n)
{
#ifdef __ZEPHYR__
  int prio = 6;

  if (iov == NULL || n <= 0) {
    return 0;
  }

  for (int i = 0; i < n; i++) {
    const char *s = iov[i].iov_base;
    size_t len = iov[i].iov_len;

    if (s != NULL && len >= 9 && memcmp(s, "PRIORITY=", 9) == 0) {
      unsigned long v = strtoul(s + 9, NULL, 10);
      if (v <= 7UL) {
        prio = (int)v;
      }
    }
  }

  static const char *const pri_tag[] = {
    "EMRG", "ALRT", "CRIT", "ERR ", "WARN",
    "NOTE", "INFO", "DBG ",
  };
  const char *tag = "INFO";

  if (prio >= 0 && prio <= 7) {
    tag = pri_tag[prio];
  }

  printk("[lg2:%s] ", tag);
  for (int i = 0; i < n; i++) {
    const char *s = iov[i].iov_base;
    size_t len = iov[i].iov_len;

    if (s == NULL || len == 0) {
      continue;
    }
    
    printk("%.*s", (int)len, s);
    if (i < n - 1) {
      printk(" | "); 
    }
  }
  printk("\n");

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