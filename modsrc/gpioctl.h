#include <linux/ioctl.h>

#define DEVICE_NAME	    "gpioctl"
#define DEVICE_PATH	    "/dev/gpioctl"

#define GPIO_IOC_MAGIC	'x'
#define IOCTL_SET_CFG	_IOW(GPIO_IOC_MAGIC, 0, int)
#define IOCTL_SET_PIN	_IOW(GPIO_IOC_MAGIC, 1, int)
#define IOCTL_GET_PIN	_IOR(GPIO_IOC_MAGIC, 2, int)
#define GPIO_IOC_MAXNR	3

#ifdef DEBUG
#ifdef __KERNEL__
#define DEBUG_NUM(a) 	printk(KERN_INFO "[%s:%d] flag=%d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...) printk(KERN_INFO "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)

#else
#define DEBUG_NUM(a) 	printf("[%s:%d] flag=%d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...) printf("[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#endif

#else
#define DEBUG_NUM(a)
#define DEBUG_INFO(fmt, args...)
#endif

struct gpio_spec {
    unsigned int pin;
    unsigned int val;
};


