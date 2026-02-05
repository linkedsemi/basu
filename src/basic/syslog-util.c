/* SPDX-License-Identifier: LGPL-2.1+ */

#include <string.h>

/* Define LOG_* constants directly here to ensure they're available for array initialization */
#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#include "macro.h"
#include "string-table.h"
#include "syslog-util.h"
#include "log.h"

static const char *const log_level_table[] = {
        [0] = "emerg",  /* LOG_EMERG */
        [1] = "alert",  /* LOG_ALERT */
        [2] = "crit",   /* LOG_CRIT */
        [3] = "err",    /* LOG_ERR */
        [4] = "warning",/* LOG_WARNING */
        [5] = "notice", /* LOG_NOTICE */
        [6] = "info",   /* LOG_INFO */
        [7] = "debug"   /* LOG_DEBUG */
};

DEFINE_STRING_TABLE_LOOKUP_WITH_FALLBACK(log_level, int, LOG_DEBUG);