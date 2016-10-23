#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/hrtimer.h>

#define DEBUG    1 
#include <linux/platform_device.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>

#include <plat/regs-spi.h>
#include <mach/regs-gpio.h>

/* gpio configuration */
struct gpio_desc {
    char *name;
    int pin;
    int pin_cfg;
    int init_val;
    int irq;
};

#define INDEX_CS        0
#define INDEX_MISO      1
#define INDEX_MOSI      2
#define INDEX_CLK       3
#define GPIO_CNT        4

static struct gpio_desc spi_gpio[] = {
    {"CS",   S3C2410_GPL(13), S3C2410_GPIO_OUTPUT, 1, -1},
    {"MISO", S3C2410_GPE(11), S3C2410_GPIO_INPUT , 0, -1},
    {"MOSI", S3C2410_GPE(12), S3C2410_GPIO_OUTPUT, 0, -1},
    {"CLK",  S3C2410_GPE(13), S3C2410_GPIO_OUTPUT, 1, -1}
};

#ifdef DEBUG
#define DEBUG_NUM(a)    printk(KERN_INFO "[%s:%d] "#a" = %d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...) printk(KERN_INFO "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_NUM(a)
#define DEBUG_INFO(fmt, args...)
#endif

static struct spi_master *spi0_controller;
static struct completion done;
static struct hrtimer timer;

struct s3c_spi_gpio {
    unsigned int cs;
    unsigned int miso;
    unsigned int mosi;
    unsigned int clk;
};

struct s3c_spi_info {
    struct spi_transfer *cur_t;
    int cur_cnt;
    struct platform_device *pdev;
    struct s3c_spi_gpio pin;
};

static int set_cs(struct s3c_spi_gpio *pin, int flag)
{
    // valid flag value is 0 or 1
    if ((flag & 0xFE) || (pin == NULL))
    return -EINVAL;

    s3c2410_gpio_setpin(pin->cs, flag & 0x01);
    return 0;
}

static enum hrtimer_restart hrtimer_func(struct hrtimer *time)
{
    complete(&done);
    return HRTIMER_NORESTART;
}

static int transmitb(unsigned char data, struct s3c_spi_gpio *pin)
{
    int bitcnt;
    unsigned int mosi;

    if (pin == NULL)
    return -EINVAL;

    s3c2410_gpio_setpin(pin->clk, 1);
    s3c2410_gpio_setpin(pin->mosi, 0);

    data &= 0xff;
    for (bitcnt=7; bitcnt>=0; bitcnt--){
        mosi = (data >> bitcnt) & 0x01;
        s3c2410_gpio_setpin(pin->mosi, mosi);  // MSB
        s3c2410_gpio_setpin(pin->clk, 0);
        s3c2410_gpio_setpin(pin->clk, 1);
    }

    s3c2410_gpio_setpin(pin->clk, 1);
    s3c2410_gpio_setpin(pin->mosi, 1);
    return 0;
}
//////////////////////////////////////////////////////////////////////////////

static int s3c2416_spi_setup(struct spi_device *spi)
{
    int index = 0;
    struct s3c_spi_info *info = spi_master_get_devdata(spi->master);
    struct s3c_spi_gpio *pin = &info->pin;
    
    pin->cs   = spi_gpio[INDEX_CS  ].pin;
    pin->miso = spi_gpio[INDEX_MISO].pin;
    pin->mosi = spi_gpio[INDEX_MOSI].pin;
    pin->clk  = spi_gpio[INDEX_CLK ].pin;

    // initial gpio configuration
    for (index = 0; index < GPIO_CNT; index++){
        s3c2410_gpio_cfgpin(spi_gpio[index].pin, spi_gpio[index].pin_cfg);
        s3c2410_gpio_setpin(spi_gpio[index].pin, spi_gpio[index].init_val);  // ccd stop
    }

    return 0;
}

static int s3c2416_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
    int err = 0;
    struct spi_master *master = spi->master;
    struct s3c_spi_info *info = spi_master_get_devdata(master);
    struct spi_transfer *t = NULL;

    set_cs(&info->pin, 0);

    list_for_each_entry (t, &mesg->transfers, transfer_list) {
        info->cur_t = t;
        info->cur_cnt = 0;
        init_completion(&done);
        hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        timer.function = hrtimer_func;

        if (t->tx_buf){
            for (; info->cur_cnt < t->len; info->cur_cnt++){
                err = transmitb(((unsigned char *)t->tx_buf)[0], &info->pin);
                if (err){
                    printk(KERN_ERR "S3C2416 GPIO SPI: Faild to transmit data!");
                    return err;
                }
            }
        }
    }

    set_cs(&info->pin, 1);

    mesg->status = 0;
    mesg->complete(mesg->context);
    return 0;
}

static struct spi_master * create_spi_gpio_master(void)
{
    int err;
    struct spi_master *master;
    struct s3c_spi_info *info;
    struct platform_device *pdev;

    pdev = platform_device_alloc("s3c2410_spi", 0);
    platform_device_add(pdev);

    master = spi_alloc_master(&pdev->dev, sizeof(struct s3c_spi_info));
    master->bus_num = 0;
    master->num_chipselect = 0xffff;
    master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
    master->setup = s3c2416_spi_setup;
    master->transfer = s3c2416_spi_transfer;

    info = spi_master_get_devdata(master);
    info->pdev = pdev;

    err = spi_register_master(master);
    if (err){
        printk("s3c2416 spi master: Failed to register master!, return value: %d\n", err);
        return NULL;
    }

    return master;
}

static void destroy_spi_master(struct spi_master *master)
{
    struct s3c_spi_info *info = spi_master_get_devdata(master);

    spi_unregister_master(master);
    platform_device_del(info->pdev);
    platform_device_put(info->pdev);
}

static int s3c2416_spi_init(void)
{
    spi0_controller = create_spi_gpio_master();
    if (spi0_controller != NULL){
        printk("s3c2416 spi master: s3c2416 spi module installed\n");
    }

    return 0;
}

static void s3c2416_spi_exit(void)
{
    destroy_spi_master(spi0_controller);
    printk("s3c2416 spi master: s3c2416 spi module removed\n");
}

module_init(s3c2416_spi_init);
module_exit(s3c2416_spi_exit);
MODULE_DESCRIPTION("S3C2416 SPI Controller Driver");
MODULE_LICENSE("GPL");
