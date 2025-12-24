
#include <zephyr/sys/byteorder.h>
#ifndef __BYTESWAP_H__
#define __BYTESWAP_H__

#ifndef bswap_16
#define bswap_16(x) BSWAP_16(x)
#endif
#ifndef bswap_32
#define bswap_32(x) BSWAP_32(x)
#endif
#ifndef bswap_64
#define bswap_64(x) BSWAP_64(x)
#endif

#ifndef htole16
#define htole16(x) sys_cpu_to_le16((uint16_t)(x))
#endif
#ifndef htole32
#define htole32(x) sys_cpu_to_le32((uint32_t)(x))
#endif
#ifndef htole64
#define htole64(x) sys_cpu_to_le64((uint64_t)(x))
#endif

#ifndef le16toh
#define le16toh(x) sys_le16_to_cpu((uint16_t)(x))
#endif
#ifndef le32toh
#define le32toh(x) sys_le32_to_cpu((uint32_t)(x))
#endif
#ifndef le64toh
#define le64toh(x) sys_le64_to_cpu((uint64_t)(x))
#endif

#ifndef be32toh
#define be32toh(x) bswap_32(x)
#endif

#endif