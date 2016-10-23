#ifndef _GPIOCTL_H_
#define _GPIOCTL_H_

#include <linux/ioctl.h>

#include "common.h"

#define DEVICE_NAME	    "gpioctl"
#define DEVICE_PATH	    "/dev/gpioctl"

#define GPIO_IOC_MAGIC	'x'
#define IOCTL_SET_CFG	_IOW(GPIO_IOC_MAGIC, 0, int)
#define IOCTL_SET_PIN	_IOW(GPIO_IOC_MAGIC, 1, int)
#define IOCTL_GET_PIN	_IOR(GPIO_IOC_MAGIC, 2, int)
#define GPIO_IOC_MAXNR	3

struct gpio_spec {
    unsigned int pin;
    unsigned int val;
};

#endif
