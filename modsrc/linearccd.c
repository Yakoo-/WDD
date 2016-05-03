#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
//#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/fiq.h>
#include <asm/irq.h>
#include <plat/fiq.h>

#include "ccd_s3c2416_fiq.h"

#define DEVICE_NAME	    "linear-ccd"

#ifdef DEBUG
#define DEBUG_LINE(a) 	printk(KERN_DEBUG "[%s:%d] flag=%d\r\n",__func__,__LINE__,a) 
#define DEBUG_INFO(fmt, args...) printk(KERN_DEBUG "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_LINE(a)
#define DEBUG_INFO(fmt, args...)
#endif

/* gpio configuration */ 
struct gpio_desc {
    char *name;
    int pin;
    int pin_cfg;
    int irq;
    int pull;
};

#define INDEX_START	0
#define INDEX_TRIGD	1

#ifdef DEBUG
#define INDEX_BUZZ	2
#define INDEX_DATIN	3
#define GPIO_CNT	4
#else
#define INDEX_DATIN	2
#define GPIO_CNT	3
#endif

static struct gpio_desc ccd_gpio[] = {
    {"Start",  S3C2410_GPF(5), S3C2410_GPIO_OUTPUT, -1, 1},
    {"TrigD",  S3C2410_GPF(3), S3C2410_GPF3_EINT3, IRQ_EINT3, -1},
#ifdef DEBUG
    {"Buzzer", S3C2410_GPF(0), S3C2410_GPIO_OUTPUT , -1, 0},
#endif
    {"DataIn", S3C2410_GPG(4), S3C2410_GPIO_INPUT , -1, 0}
};

/* definitions for fiq handler */
struct fiq_code {
    unsigned int length;
    unsigned int data[0];
};
extern struct fiq_code ccd_s3c2416_fiq_handler;

struct fiq_args {
    unsigned int status;
    unsigned int *va_irq;
    unsigned int *va_gpio;
};

/* global variables */
static char __initdata banner[] = "\nS10077 linear CCD Driver(FIQ mode), By Kerwin.Yan\n\n";
static unsigned int *pixels_buffer;
static struct fiq_args *fiq_arg;
static struct class *ccd_class;
static struct device *ccd_dev;
static unsigned int major;

static struct fiq_handler ccd_s3c2416_fiq_fh = {
    .name   = "ccd_s3c2416_fiq_handler"
};

// config gpio and register irq 
static int ccd_open(struct inode *inode, struct file *file)
{
    int ret;
    struct pt_regs regs;
    struct fiq_code *codes = &ccd_s3c2416_fiq_handler;

    // alloc 1 page (4KB) memory for pixels_buffer
    pixels_buffer = (unsigned int *)get_zeroed_page(GFP_KERNEL);
    if (pixels_buffer == NULL){
	printk("Failed to alloc new page for pixels_buffer!\n");
	goto err_buffer;
    }

    // claim fiq
    ret = claim_fiq(&ccd_s3c2416_fiq_fh);
    if (ret){
	printk("Failed to claim fiq!\n");
	goto err_claim;
    }

    // setup fiq regs
    regs.uregs[reg_args]  = (long)fiq_arg;
    regs.uregs[reg_buffer]= (long)pixels_buffer;
    regs.uregs[reg_tmpd]  = 0;
    regs.uregs[reg_tmpa]  = 0;
    regs.uregs[reg_index] = 0;
    regs.uregs[reg_datain]= 0;

    set_fiq_regs(&regs);
    set_fiq_handler(&codes->data, codes->length);

    irq_set_irq_type(IRQ_NUM, IRQF_TRIGGER_FALLING);
    s3c24xx_set_fiq(IRQ_NUM, 1);
    enable_irq(IRQ_NUM);

#ifdef DEBUG
    DEBUG_INFO("fiq_arg->status: %d, fiq_arg->va_irq: 0x%p, fiq_arg->va_gpio: 0x%p", 
	    fiq_arg->status, fiq_arg->va_irq, fiq_arg->va_gpio);
    DEBUG_INFO("S3C24XX_VA_IRQ: 0x%p", S3C24XX_VA_IRQ);
    DEBUG_INFO("S3C24XX_VA_GPIO: 0x%p", S3C24XX_VA_GPIO);
#endif

    printk("FIQ codes inserted, code length: %d Bytes\n", codes->length);
    return 0;

err_claim:
    free_page((unsigned long)pixels_buffer);
err_buffer:
    return -EINVAL;
}

static int ccd_close(struct inode *inode, struct file *file)
{
    disable_irq(IRQ_NUM);
    s3c24xx_set_fiq(IRQ_NUM, 0);
    release_fiq(&ccd_s3c2416_fiq_fh);
    free_page((unsigned long)pixels_buffer);
    return 0;
}

static int ccd_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
    int timeout = 0, ret;

    // enable receive procedure
    fiq_arg->status = FIQ_STATUS_RUNNING;
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 0);

    while((fiq_arg->status != FIQ_STATUS_FINISHED) && (timeout < 10)){
	msleep(100);
	timeout++;
    }

    if (fiq_arg->status != FIQ_STATUS_FINISHED){
	// failed to receive data from linear ccd, clear all global registers
	memset(pixels_buffer, 0, CCD_BUFFER_SIZE);
	printk("Failed to receive image data, time out!\n");
	return -EBUSY;
    }

    ret = copy_to_user(buff, pixels_buffer, CCD_BUFFER_SIZE);
    memset(pixels_buffer, 0, CCD_BUFFER_SIZE);

    // disable receive procedure
    fiq_arg->status = FIQ_STATUS_STOPPED;
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 1);

    return ret ? -EFAULT : CCD_BUFFER_SIZE;
}

static struct file_operations ccd_fops = {
    .owner  = THIS_MODULE,
    .open   = ccd_open,
    .release= ccd_close,
    .read   = ccd_read,
};

static int __init ccd_init(void)
{
    int index = 0;

    printk(banner);
    
    // initial fiq_args structure
    fiq_arg = (struct fiq_args *)kmalloc(sizeof(struct fiq_args), GFP_KERNEL);
    if (fiq_arg == NULL){
	printk("Failed to alloc memory for fiq_arg!\n");
	goto err_arg;
    }
    fiq_arg->status = FIQ_STATUS_STOPPED;
    fiq_arg->va_irq  = S3C24XX_VA_IRQ;
    fiq_arg->va_gpio = S3C24XX_VA_GPIO;

    // regist a file operation
    major = register_chrdev(0,DEVICE_NAME,&ccd_fops);
    if (major < 0) {
	printk("can't register major number!\n");
	goto err_major;
    }

    // make a device class
    ccd_class = class_create(THIS_MODULE,DEVICE_NAME);
    if (IS_ERR(ccd_class)){
	printk("Error: failed to create ccd_class! \n");
	goto err_class;
    }

    // make a device node
    ccd_dev = device_create(ccd_class,NULL,MKDEV(major,0),NULL,DEVICE_NAME);
    if (IS_ERR(ccd_dev)){
	printk("Error: failed to create ccd_device!\n");
	goto err_dev;
    }

    // initial gpio configuration
    for (index=0; index<GPIO_CNT; index++) {
	s3c2410_gpio_cfgpin(ccd_gpio[index].pin, ccd_gpio[index].pin_cfg);
#if 0
	if (ccd_gpio[index].pull >= 0)
	    s3c2410_gpio_pullup(ccd_gpio[index].pin,
				ccd_gpio[index].pull);
#endif
    }
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 1);
#ifdef DEBUG
    s3c2410_gpio_setpin(ccd_gpio[INDEX_BUZZ].pin, 1);
#endif


    printk(DEVICE_NAME " initialized.\n\n");
    return 0;

err_dev :
    class_destroy(ccd_class);
err_class :
    unregister_chrdev(major,DEVICE_NAME);
err_major :
    kfree(fiq_arg);
err_arg :
    return -EBUSY;
}

static void __exit ccd_exit(void)
{
    device_destroy(ccd_class, MKDEV(major,0));
    class_destroy(ccd_class);
    unregister_chrdev(major, DEVICE_NAME);
    kfree(fiq_arg);
}

module_init(ccd_init);
module_exit(ccd_exit);
MODULE_LICENSE("GPL");
