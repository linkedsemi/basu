/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#ifdef __ZEPHYR__

/* Only define ftrylockfile if it's not already defined */
#ifndef ftrylockfile
#include <sys/lock.h>
#include <sys/reent.h>

int ftrylockfile(FILE *fp)
{
    if (fp && !(fp->_flags & __SSTR)) {
        return __lock_try_acquire_recursive(fp->_lock);
    }
    return 0;
}
#endif

#endif /* __ZEPHYR__ */