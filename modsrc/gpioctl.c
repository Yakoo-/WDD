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

#define DEVICE_NAME	"gpioctl"

#define IOCTL_SET_CFG	1
#define IOCTL_SET_PIN	2
#define IOCTL_GET_PIN	3	

#ifdef DEBUG
#define DEBUG_LINE(a) 	printk(KERN_INFO "[%s:%d] flag=%d\r\n",__func__,__LINE__,a) 
// do not end up with \n
#define DEBUG_INFO(fmt, args...) printk(KERN_INFO "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_LINE(a)
#define DEBUG_INFO(fmt, args...)
#endif

static int major;   // device major number is assigned by the system
static struct class *gpio_class;
static struct device *gpio_dev;

static int gpio_open(struct inode *inode, struct file *file)
{
    return 0;
}

static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    DEBUG_INFO("cmd: 0x%X, arg: %ld", cmd, arg);
    switch (cmd & 0x0f)
    {
	case IOCTL_SET_CFG:
	    s3c2410_gpio_cfgpin(arg, (cmd & 0xf0) >> 4);
	    return 0;
	case IOCTL_SET_PIN:
	    s3c2410_gpio_setpin(arg, (cmd & 0xf0) >> 4);
	    return 0;
	case IOCTL_GET_PIN:
	    return s3c2410_gpio_getpin(arg);
	default:
	    return -EINVAL;
    }
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
