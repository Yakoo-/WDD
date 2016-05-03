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

#define DEVICE_NAME	"fiqtest"

#if 0
#define	FIQ_IOC_MAGIC		'y'
#define FIQ_START		_IO(FIQ_IOC_MAGIC, 0)
#define FIQ_STOP		_IO(FIQ_IOC_MAGIC, 1)
#define FIQ_RESET		_IO(FIQ_IOC_MAGIC, 2)
#endif

#ifdef DEBUG
#define DEBUG_INFO(fmt, args...) printk(KERN_DEBUG "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_INFO(fmt, args...)
#endif

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

static char __initdata banner[] = "\nCOM2416 fiqtests, By Kerwin Yan\n\n";
static unsigned int *fiq_buffer;
static struct fiq_args *fiq_arg;

static struct fiq_handler ccd_s3c2416_fiq_fh = {
    .name   = "ccd_s3c2416_fiq_handler"
};


static int __init fiqtest_init(void)
{
    int ret, index;
    struct pt_regs regs;
    struct fiq_code *codes = &ccd_s3c2416_fiq_handler;

    printk(banner);
    
    // initial fiq_args structure
    fiq_arg = (struct fiq_args *)kmalloc(sizeof(struct fiq_args), GFP_KERNEL);
    if (fiq_arg == NULL){
	printk(DEVICE_NAME "Failed to alloc memory for fiq_arg!\n");
	goto err_arg;
    }
    fiq_arg->status = 0;
    fiq_arg->va_irq  = S3C24XX_VA_IRQ;
    fiq_arg->va_gpio = S3C24XX_VA_GPIO;

    // alloc 1 page (4KB) memory for fiq_buffer
    fiq_buffer = (unsigned int *)get_zeroed_page(GFP_KERNEL);
    if (fiq_buffer == NULL){
	printk(DEVICE_NAME "Failed to alloc new page for fiq_buffer!\n");
	goto err_buffer;
    }

    // initial gpio configuration
    for (index=0; index<GPIO_CNT; index++) {
	s3c2410_gpio_cfgpin(ccd_gpio[index].pin, ccd_gpio[index].pin_cfg);
//	if (ccd_gpio[index].pull >= 0)
//	    s3c2410_gpio_pullup(ccd_gpio[index].pin,
//				ccd_gpio[index].pull);
    }
    s3c2410_gpio_setpin(ccd_gpio[INDEX_START].pin, 1);
#ifdef DEBUG
    s3c2410_gpio_setpin(ccd_gpio[INDEX_BUZZ].pin, 0);
#endif

    // claim fiq
    ret = claim_fiq(&ccd_s3c2416_fiq_fh);
    if (ret){
	printk(DEVICE_NAME "Failed to claim fiq!\n");
	goto err;
    }

    // setup fiq regs
    // enable receive procedure
    fiq_arg->status = 1;
    regs.uregs[reg_args]  = (long)fiq_arg;
    regs.uregs[reg_buffer]= (long)fiq_buffer;
    regs.uregs[reg_tmpd]  = 0;
    regs.uregs[reg_tmpa]  = 0;
    regs.uregs[reg_index] = 0;
    regs.uregs[reg_datain]= 0;

    set_fiq_regs(&regs);

    set_fiq_handler(&codes->data, codes->length);
    printk(DEVICE_NAME " Fiq codes inserted, code length: %d Bytes\n", codes->length);

    irq_set_irq_type(IRQ_NUM, IRQF_TRIGGER_FALLING);
    s3c24xx_set_fiq(IRQ_NUM, 1);
    enable_irq(IRQ_NUM);

    DEBUG_INFO("fiq_arg->status: %d, fiq_arg->va_irq: 0x%p, fiq_arg->va_gpio: 0x%p", fiq_arg->status, fiq_arg->va_irq, fiq_arg->va_gpio);
    DEBUG_INFO("S3C24XX_VA_IRQ: 0x%p", S3C24XX_VA_IRQ);
    DEBUG_INFO("S3C24XX_VA_GPIO: 0x%p", S3C24XX_VA_GPIO);

    printk(DEVICE_NAME " initialized!\n");
    return 0;

err:
    free_page((unsigned long)fiq_buffer);
err_buffer:
    kfree(fiq_arg);
err_arg:
    return -EINVAL;
}

static void __exit fiqtest_exit(void)
{
    kfree(fiq_arg);
    free_page((unsigned long)fiq_buffer);
    s3c24xx_set_fiq(IRQ_NUM, 0);
    release_fiq(&ccd_s3c2416_fiq_fh);
    disable_irq(IRQ_NUM);
}

module_init(fiqtest_init);
module_exit(fiqtest_exit);

MODULE_LICENSE("GPL");

