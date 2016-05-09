#define DEBUG 1

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <mach/regs-gpio.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>


#include "gpioctl.h"
 
static int major;   // device major number is assigned by the system
static struct class *gpio_class;
static struct device *gpio_dev;

static int gpio_open(struct inode *inode, struct file *file)
{
    return 0;
}
 
static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int err = 0, ret = 0;
    struct gpio_spec pin_spec;

    if (_IOC_TYPE(cmd) != GPIO_IOC_MAGIC)
	return - ENOTTY;
    if (_IOC_NR(cmd) >= GPIO_IOC_MAXNR)
	return - ENOTTY;

    err = copy_from_user((void *)&pin_spec, (void __user *)arg, sizeof(struct gpio_spec));
    if(err){
	printk(KERN_ERR "Failed to copy data from user!");
	return - EFAULT;
    }
    DEBUG_INFO("pin: %d, val: %d", pin_spec.pin, pin_spec.val);

    switch (cmd & 0x0f)
    {
	case IOCTL_SET_CFG:
	    s3c2410_gpio_cfgpin(pin_spec.pin, pin_spec.val);
	    break;
	case IOCTL_SET_PIN:
	    s3c2410_gpio_setpin(pin_spec.pin, pin_spec.val);
	    break;
	case IOCTL_GET_PIN:
	    pin_spec.val = s3c2410_gpio_getpin(pin_spec.pin);
	    ret = 0;
	    ret = copy_to_user((void __user *)arg, (void *)&pin_spec, sizeof(struct gpio_spec));
	    break;
	default:
	    return - ENOTTY;
    }
    return ret;
}

static struct file_operations gpio_fops=
{
    .owner  = THIS_MODULE,
    .open   = gpio_open,
    .unlocked_ioctl  = gpio_ioctl,
};


static int __init gpio_init(void)
{
    // regist a file operation
    major = register_chrdev(0,DEVICE_NAME,&gpio_fops);
    if (major < 0) {
	printk(DEVICE_NAME ": can't register major number!\n");
	goto err_major;
    }

    // make a device class
    gpio_class = class_create(THIS_MODULE,DEVICE_NAME);
    if (IS_ERR(gpio_class)){
	printk("Error: failed to create gpio_class! \n");
	goto err_class;
    }

    // make a device node
    gpio_dev = device_create(gpio_class,NULL,MKDEV(major,0),NULL,DEVICE_NAME);
    if (IS_ERR(gpio_dev)){
	printk("Error: failed to create gpio_device!\n");
	goto err_dev;
    }

    printk(DEVICE_NAME ": initialized.\n");
    return 0;

err_dev:
    class_destroy(gpio_class);
err_class:
    unregister_chrdev(major,DEVICE_NAME);
err_major:
    return -EBUSY;
}

static void __exit gpio_exit(void)
{
    device_destroy(gpio_class,MKDEV(major,0));
    class_destroy(gpio_class);
    unregister_chrdev(major,DEVICE_NAME);
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");
