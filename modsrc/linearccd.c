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
#define DEBUG_NUM(a) 	printk(KERN_DEBUG "[%s:%d] "#a" = %d\r\n",__func__,__LINE__,a) 
#define DEBUG_INFO(fmt, args...) printk(KERN_DEBUG "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_NUM(a)
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

#define INDEX_START	    0
#define INDEX_TRIGD	    1
#define INDEX_DATIN	    2
#define INDEX_LEDCT     3
#define GPIO_CNT        4	

static struct gpio_desc ccd_gpio[] = {
    {"Start",  S3C2410_GPF(5), S3C2410_GPIO_OUTPUT , -1, 1},
    {"TrigD",  S3C2410_GPF(3), S3C2410_GPF3_EINT3  , IRQ_EINT3, -1},
    {"DataIn", S3C2410_GPG(4), S3C2410_GPIO_INPUT  , -1, 0},
    {"LEDCtr", S3C2410_GPE(15), S3C2410_GPIO_OUTPUT , -1, 0}

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
    struct fiq_code *codes = &ccd_s3c2416_fiq_handler;

    // claim fiq
    ret = claim_fiq(&ccd_s3c2416_fiq_fh);
    if (ret){
        printk("Failed to claim fiq!\n");
        goto err_claim;
    }

    set_fiq_handler(&codes->data, codes->length);

    irq_set_irq_type(IRQ_NUM, IRQF_TRIGGER_FALLING);
    s3c24xx_set_fiq(IRQ_NUM, 1);
    enable_irq(IRQ_NUM);

    DEBUG_INFO("fiq_arg: 0x%p", fiq_arg);
    DEBUG_INFO("buffer : 0x%p", pixels_buffer);
    DEBUG_INFO("S3C24XX_VA_IRQ: 0x%p", S3C24XX_VA_IRQ);
    DEBUG_INFO("S3C24XX_VA_GPIO: 0x%p", S3C24XX_VA_GPIO);

    printk("FIQ codes inserted, code length: %d Bytes\n", codes->length);
    return 0;

err_claim:
    return -EINVAL;
}

static int ccd_close(struct inode *inode, struct file *file)
{
    release_fiq(&ccd_s3c2416_fiq_fh);
    s3c24xx_set_fiq(IRQ_NUM, 0);
    disable_irq(IRQ_NUM);
    return 0;
}

static int ccd_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
    int ret;
#ifdef DEBUG
    struct pt_regs regs;
#endif

    // clear the buffer
    memset(pixels_buffer, 0, CCD_BUFFER_SIZE);

    // reset all fiq registers before each time we read data from ccd
    regs.uregs[reg_args]  = (long)fiq_arg;
    regs.uregs[reg_buffer]= (long)pixels_buffer;
    regs.uregs[reg_tmpd]  = 0;
    regs.uregs[reg_tmpa]  = 0;
    regs.uregs[reg_index] = 0;
    regs.uregs[reg_datain]= 0;
    set_fiq_regs(&regs);
 
    // enable receive procedure
    fiq_arg->status = FIQ_STATUS_RUNNING;
    s3c2410_gpio_setpin(ccd_gpio[INDEX_LEDCT].pin, 1);  // light led
    msleep(10);
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 0);  // ccd start
    msleep(100);    // wait for data ready
    // disable receive procedure
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 1);
    s3c2410_gpio_setpin(ccd_gpio[INDEX_LEDCT].pin, 0);

    // check return value
    get_fiq_regs(&regs);
    DEBUG_INFO("status: %d"  ,*(unsigned int *)regs.uregs[reg_args]);
    DEBUG_INFO("buffer: 0x%p", (unsigned int *)regs.uregs[reg_buffer]);
    DEBUG_INFO("tempd : %d"  , (unsigned int  )regs.uregs[reg_tmpd]);
    DEBUG_INFO("tempa : 0x%p", (unsigned int *)regs.uregs[reg_tmpa]);
    DEBUG_INFO("index : 0x%p", (unsigned int *)regs.uregs[reg_index]);
    DEBUG_INFO("datain: 0x%p", (unsigned int *)regs.uregs[reg_datain]);

    if (fiq_arg->status != FIQ_STATUS_FINISHED){
        // failed to receive data from linear ccd
        if (regs.uregs[reg_index] != 0x03ff0009){
            memset(pixels_buffer, 0, CCD_BUFFER_SIZE);
            printk("Failed to receive image data, time out!\n");
            return -EBUSY;
        }
        // we lost the last bit of ccd data, manually ignore this bit
        pixels_buffer[CCD_MAX_PIXEL_CNT - 1] = regs.uregs[reg_datain] << 1;
        DEBUG_INFO("We losted the last bit! Ignoring it!");
    }

    ret = copy_to_user(buff, pixels_buffer, CCD_BUFFER_SIZE);
    memset(pixels_buffer, 0, CCD_BUFFER_SIZE);
    fiq_arg->status = FIQ_STATUS_STOPPED;

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
 
    // alloc 1 page (4KB) memory for pixels_buffer
    pixels_buffer = (unsigned int *)get_zeroed_page(GFP_KERNEL);
    if (pixels_buffer == NULL){
	printk("Failed to alloc new page for pixels_buffer!\n");
	goto err_buffer;
    }
   
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
            s3c2410_gpio_pullup(ccd_gpio[index].pin, ccd_gpio[index].pull);
#endif
    }
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 1);  // ccd stop

    printk(DEVICE_NAME " initialized.\n\n");
    return 0;

err_dev :
    class_destroy(ccd_class);
err_class :
    unregister_chrdev(major,DEVICE_NAME);
err_major :
    kfree(fiq_arg);
err_arg :
    free_page((unsigned long)pixels_buffer);
err_buffer:
    return -EBUSY;
}

static void __exit ccd_exit(void)
{
    device_destroy(ccd_class, MKDEV(major,0));
    class_destroy(ccd_class);
    unregister_chrdev(major, DEVICE_NAME);
    kfree(fiq_arg);
    free_page((unsigned long)pixels_buffer);
}

module_init(ccd_init);
module_exit(ccd_exit);
MODULE_LICENSE("GPL");
