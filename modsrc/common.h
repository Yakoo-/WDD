#ifndef _COMMON_H_
#define _COMMON_H_

#define DEBUG   1

#ifdef DEBUG
#ifdef __KERNEL__
#define DEBUG_NUM(a)                printk(KERN_INFO "[%s:%d] "#a" = %d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...)    printk(KERN_INFO "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)

#else
#define DEBUG_NUM(a)                printf("[%s:%d] "#a" = %d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...)    printf("[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#endif

#else
#define DEBUG_NUM(a)
#define DEBUG_INFO(fmt, args...)
#endif

#define uchar       unsigned char
#define uchar8      unsigned char
#define ushort      unsigned short
#define ushort16    unsigned short
#define uint        unsigned int
#define uint32      unsigned int
#define int64       long long
#define uint64      unsigned long long

#endif
