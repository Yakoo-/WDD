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

#define	DEVICE_NAME	"led"
#define LED_MAJOR	231
#define IOCTL_LED_ON	1
#define	IOCTL_LED_OFF	0

static unsigned long led_pin = S3C2410_GPC(0);
static unsigned int  led_cfg = S3C2410_GPIO_OUTPUT;

static int led_open(struct inode *inode, struct file *file)
{
    s3c2410_gpio_cfgpin(led_pin,led_cfg);
    return 0;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
	case IOCTL_LED_ON:
	    s3c2410_gpio_setpin(led_pin,0);
	    return 0;
	case IOCTL_LED_OFF:
	    s3c2410_gpio_setpin(led_pin,1);
	    return 0;
	default:
	    return -EINVAL;
    }
}

static struct file_operations led_fops=
{
    .owner  = THIS_MODULE,
    .open   = led_open,
    .unlocked_ioctl  = led_ioctl,
};

static struct class *led_class;

static int __init led_init(void)
{
    int ret;

    ret = register_chrdev(LED_MAJOR,DEVICE_NAME,&led_fops);

    if (ret < 0)
    {
	printk(DEVICE_NAME ": can't register major number!\n");
	return ret;
    }

    led_class = class_create(THIS_MODULE,DEVICE_NAME);
    if(IS_ERR(led_class))
    {
	printk("Err: failed during create led_class. \n");
	return -1;
    }

    device_create(led_class,NULL,MKDEV(LED_MAJOR,0),NULL,DEVICE_NAME);

    printk(DEVICE_NAME ": initialized.\n");
    return 0;
}

static void __exit led_exit(void)
{
    unregister_chrdev(LED_MAJOR,DEVICE_NAME);
    device_destroy(led_class,MKDEV(LED_MAJOR,0));
    class_destroy(led_class);
}

module_init(led_init);
module_exit(led_exit);


MODULE_LICENSE("GPL");

