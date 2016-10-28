#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/clk.h>

#include <mach/regs-gpio.h>
#include <plat/regs-adc.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#define DEVICE_NAME    "adc"

static void __iomem *base_addr;

typedef struct
{
    wait_queue_head_t wait;
    int channel;
    int prescale;
}ADC_DEV;

//DECLARE_MUTEX(ADC_LOCK);  // for linux 2.6.36 and earlier
DEFINE_SEMAPHORE(ADC_LOCK);


/*  in linux/semaphore.h line 29
#define DEFINE_SEMAPHORE(name)    \
    struct semaphore name = __SEMAPHORE_INITIALIZER(name, 1)
*/

static int ADC_enable = 0;
static ADC_DEV adcdev;
static volatile int ev_adc = 0;
static int adc_data;

static struct clk    *adc_clock;

#define ADCCON          (*(volatile unsigned long *)(base_addr + S3C2410_ADCCON))
#define ADCTSC          (*(volatile unsigned long *)(base_addr + S3C2410_ADCTSC))    
#define ADCDLY          (*(volatile unsigned long *)(base_addr + S3C2410_ADCDLY))    
#define ADCDAT0         (*(volatile unsigned long *)(base_addr + S3C2410_ADCDAT0))    
#define ADCDAT1         (*(volatile unsigned long *)(base_addr + S3C2410_ADCDAT1))    
#define ADCUPDN         (*(volatile unsigned long *)(base_addr + S3C2410_ADCUPDN))
#define ADCMUX          (*(volatile unsigned long *)(base_addr + 0x18))

// for ADCCON
#define ENDFLAG         (1 << 15)   // read only
#define PRESCALE_DIS    (0 << 14)
#define PRESCALE_EN     (1 << 14)
#define PRSCVL(x)       (((x) & 0xff) << 6)
#define RES_12BIT       (1 << 3)
#define READ_START      (1 << 1)    // continous read
#define ENABLE_START    (1 << 0)    // read only once a time
// for ADCMUX
#define MUXSEL(x)       (((x) & 0xf) << 0)

#define START_ADC(ch, prescale)    \
    do{ ADCMUX = MUXSEL(ch); \
        ADCCON = PRESCALE_EN | PRSCVL(prescale) | RES_12BIT ; \
        ADCCON |= ENABLE_START; \
    }while(0)

#define ADC_CMD_SELMUX 0

static irqreturn_t adc_int_handler(int irq, void *dev_id)
{
    if (ADC_enable)
    {
#ifdef RES_12BIT
        adc_data = ADCDAT0 & 0xfff;
#elif
        adc_data = ADCDAT0 & 0x3ff;
#endif
        ev_adc = 1;
        wake_up_interruptible(&adcdev.wait);
    }
    return IRQ_HANDLED;
}

static ssize_t adc_read(struct file *flip, char __user *buffer, size_t count, loff_t *ppos)
{
    char str[20];
    int value;
    size_t len;

    if (down_trylock(&ADC_LOCK) == 0){
        ADC_enable = 1;
        START_ADC(adcdev.channel, adcdev.prescale);
        wait_event_interruptible(adcdev.wait, ev_adc);
        ev_adc = 0;

        printk(KERN_DEBUG "COM2416 ADC Driver: AIN[%d] = 0x%04x, %d\n", adcdev.channel, adc_data, adc_data);
        value = adc_data;
        sprintf(str, "%d", adc_data);
        ADC_enable = 0;
        up(&ADC_LOCK);
    }else{
        value = -1;
    }

    len = sprintf(str, "%d", value);
    if (count >= len){
        int r = copy_to_user(buffer, str, len);
        return r ? r : len;
    }else{
        return -EINVAL;
    }
}

static long adc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd){

    case ADC_CMD_SELMUX:
        if ((arg >= 0) && (arg <= 9)){
            adcdev.channel = arg;
            return 0;
        }else{
            return -EINVAL;
        }
    default:
        return -EINVAL;
    }
}

static int adc_open(struct inode *inode, struct file *filp)
{
    init_waitqueue_head(&(adcdev.wait));

    adcdev.channel = 0;        // defalut selection channel 0 
    adcdev.prescale = 49;   // adc_feq = PCLK/(prescale + 1); for this 1Mhz

    printk(KERN_DEBUG "ADC opened, channel:%d\n", adcdev.channel);
    return 0;
}

static int adc_release(struct inode *inode, struct file *flip)
{
    printk(KERN_DEBUG "ADC closed, channel:%d\n", adcdev.channel);
    return 0;
}

static struct file_operations dev_fops = {
    owner:        THIS_MODULE,
    open:        adc_open,
    read:        adc_read,
    release:        adc_release,
    unlocked_ioctl: adc_ioctl,
};

static struct miscdevice misc= {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &dev_fops,
};

static int __init dev_init(void)
{
    int err;

    base_addr = ioremap(S3C2410_PA_ADC, 0x22);
    if (base_addr == NULL){
    printk(KERN_ERR " Failed to remap register block\n");
    return -ENOMEM;
    }
    
    adc_clock = clk_get(NULL, "adc");
    if (IS_ERR(adc_clock)){
    printk(KERN_ERR " Failed to get adc clock source\n");
    return -ENOMEM;
    }
    clk_enable(adc_clock);

    ADCTSC = 0;

    err = request_irq(IRQ_ADC, adc_int_handler, IRQF_SHARED, DEVICE_NAME, &adcdev);
    if (err){
    iounmap(base_addr);
    return err;
    }

    err = misc_register(&misc);

    printk(DEVICE_NAME " initialized. \n");
    return err;
}

static void __exit dev_exit(void)
{
    free_irq(IRQ_ADC, &adcdev);
    iounmap(base_addr);

    if (adc_clock){
    clk_disable(adc_clock);
    clk_put(adc_clock);
    adc_clock = NULL;
    }
    
    misc_deregister(&misc);
}

EXPORT_SYMBOL(ADC_LOCK);
module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ADC Driver for COM2416 board");


