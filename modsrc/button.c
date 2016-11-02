#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <mach/regs-gpio.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "common.h"

#define DEVICE_NAME	    "button"
#define BUTTON_MAJOR	232

struct button_irq_desc
{
    int irq;
    int pin;
    int pin_setting;
    int number;
    char *name;
};


static struct button_irq_desc button_irqs[] = 
{
    {IRQ_EINT9,  S3C2410_GPG(1), S3C2410_GPG1_EINT9,  0, "KEY1"},
    {IRQ_EINT10, S3C2410_GPG(2), S3C2410_GPG2_EINT10, 1, "KEY2"},
    {IRQ_EINT11, S3C2410_GPG(3), S3C2410_GPG3_EINT11, 2, "KEY3"},
    {IRQ_EINT12, S3C2410_GPG(4), S3C2410_GPG4_EINT12, 3, "KEY4"}
};

static unsigned int button_total = sizeof(button_irqs)/sizeof(button_irqs[0]);
static volatile int key_values[] = {0,0,0,0};
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
static volatile int ev_press = 0;

static irqreturn_t button_interrupt(int irq, void *dev_id)
{
    struct button_irq_desc *button_irqs = (struct button_irq_desc *)dev_id;
    int up = s3c2410_gpio_getpin(button_irqs->pin);

    if (up) {
        key_values[button_irqs->number] = (button_irqs->number+ 1) + 0x80;
    } else {
        key_values[button_irqs->number] = (button_irqs->number +1);
    }
    
    ev_press = 1;
    wake_up_interruptible(&button_waitq);
    
    return IRQ_RETVAL(IRQ_HANDLED);
}

static int button_open(struct inode *inode, struct file *file)
{
    int i;
    int err;

    for (i = 0;i < button_total;i++) {
        s3c2410_gpio_cfgpin(button_irqs[i].pin,button_irqs[i].pin_setting);
        err = request_irq(button_irqs[i].irq, button_interrupt, IRQ_TYPE_EDGE_BOTH, button_irqs[i].name, (void *)&button_irqs[i]);
        if (err)
            break;
    }

    if (err) {
        for(i = 3;i >= 0;i--) {
            disable_irq(button_irqs[i].irq);
            free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
        }
        return -EBUSY;
    }

    return 0;
}

static int button_close(struct inode *inode, struct file *file)
{
    int i;

    for(i = 0;i < button_total;i++) {
        disable_irq(button_irqs[i].irq);
        free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
    }

    return 0;
}

static int button_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
    unsigned long err;
    if(!ev_press) {
        if(file->f_flags & O_NONBLOCK)
            return -EAGAIN;
        else
            wait_event_interruptible(button_waitq, ev_press);
    }

    ev_press = 0;

    err = copy_to_user(buff, key_values, min(sizeof(key_values), count));
    memset((void *)key_values, 0, sizeof(key_values));

    return err ? -EFAULT : min(sizeof(key_values), count);
}

static unsigned int button_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &button_waitq, wait);
    if(ev_press)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static struct file_operations button_fops = 
{
    .owner  = THIS_MODULE,
    .open   = button_open,
    .release= button_close,
    .read   = button_read,
    .poll   = button_poll,
};

static char __initdata banner[] = "COM2416 Buttons, By Yakoo\n";
static struct class *button_class;

static int __init button_init(void)
{
    int ret;
    printk(banner);

    ret = register_chrdev(BUTTON_MAJOR, DEVICE_NAME, &button_fops);
    if (ret < 0) {
        printk(DEVICE_NAME "can't register major number\n");
        return ret;
    }

    button_class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(button_class)) {
        printk("Err: failed to create button_class.\n");
        return -1;
    }

    device_create(button_class, NULL, MKDEV(BUTTON_MAJOR, 0), NULL, DEVICE_NAME);

    printk(DEVICE_NAME " initialized\n");
    return 0;
}

static void __exit button_exit(void)
{
    unregister_chrdev(BUTTON_MAJOR, DEVICE_NAME);
    device_destroy(button_class, MKDEV(BUTTON_MAJOR, 0));
    class_destroy(button_class);

}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
