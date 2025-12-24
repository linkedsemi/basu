/* Zephyr 环境中缺失的 Linux 特有 errno 常量 */
#ifdef __ZEPHYR__


#ifndef ENOTNAM
#define ENOTNAM 3000
#endif

#ifndef EREMOTEIO  
#define EREMOTEIO 3001
#endif

#ifndef ENAVAIL
#define ENAVAIL 3002
#endif

#ifndef ERESTART
#define ERESTART 3003
#endif

#ifndef ENOKEY
#define ENOKEY 3004
#endif

#ifndef EHWPOISON
#define EHWPOISON 3005
#endif

#ifndef EISNAM
#define EISNAM 3006
#endif

#ifndef ERFKILL
#define ERFKILL 3007
#endif

#ifndef EKEYREVOKED
#define EKEYREVOKED 3008
#endif

#ifndef EKEYREJECTED
#define EKEYREJECTED 3009
#endif

#ifndef EMEDIUMTYPE
#define EMEDIUMTYPE 3010
#endif

#ifndef EUCLEAN
#define EUCLEAN 3011
#endif

#ifndef EKEYEXPIRED
#define EKEYEXPIRED 3012
#endif

#endif /* __ZEPHYR__ */