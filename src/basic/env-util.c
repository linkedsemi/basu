/* SPDX-License-Identifier: LGPL-2.1+ */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __ZEPHYR__
#include <basu_zephyr_compat.h>
#endif

#include "env-util.h"
#include "parse-util.h"

int getenv_bool(const char *p) {
        const char *e;

        e = getenv(p);
        if (!e)
                return -ENXIO;

        return parse_boolean(e);
}

#if !HAVE_SECURE_GETENV
char *secure_getenv(const char *name) {
        if (issetugid()) {
                return NULL;
        }

        return getenv(name);
}
#endif