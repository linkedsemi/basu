/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

#ifdef __FreeBSD__
#include <sys/endian.h>
#elif defined(__ZEPHYR__)
#include <zephyr/sys/byteorder.h>
#else
#include <endian.h>
#endif
#include <stdint.h>
#ifdef __ZEPHYR__
#include <zephyr/sys/byteorder.h>

static inline uint64_t unaligned_read_le64(const void *_u) {
        const struct __attribute__((packed, may_alias)) { uint64_t x; } *u = _u;

        return sys_le64_to_cpu(u->x);
}
#else
static inline uint64_t unaligned_read_be64(const void *_u) {
        const struct __attribute__((packed, may_alias)) { uint64_t x; } *u = _u;
        return be64toh(u->x);
}
#endif