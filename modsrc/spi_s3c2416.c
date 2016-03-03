#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/irq.h>

#define DEBUG    1 
#include <linux/platform_device.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>

#include <plat/regs-spi.h>
#include <mach/regs-gpio.h>

// special function registers for s3c2416 hsspi
#define S3C2416_SPI_BASE	(0x52000000)
#define S3C2416_SPI_CHCFG	(0x00)  //SPI configuration
#define S3C2416_SPI_CLKCFG	(0x04)  //Clock configuration
#define S3C2416_SPI_MODECFG	(0x08)  //SPI FIFO control
#define S3C2416_SPI_SLAVESEL	(0x0C)  //Slave selection
#define S3C2416_SPI_SPIINTEN	(0x10)  //SPI interrupt enable
#define S3C2416_SPI_SPISTATUS	(0x14)  //SPI status
#define S3C2416_SPI_SPITXDATA	(0x18)  //SPI TX data
#define S3C2416_SPI_SPIRXDATA	(0x1C)  //SPI RX data
#define S3C2416_SPI_PACKETCNT	(0x20)  //count how many data master gets
#define S3C2416_SPI_PENDINGCLR	(0x24)  //Pending clear
#define S3C2416_SPI_SWAPCFG	(0x28)  //SWAP config register
#define S3C2416_SPI_FBClkSEL	(0x2C)  //Feedback clock selecting register
#define S3C2410_GPL13_SPISS0	(0x02 << 26)

//#ifdef DEBUG
//#undef DEBUG
#ifdef DEBUG
#define DEBUG_LINE(a) 	printk(KERN_DEBUG "[%s:%d] flag=%d\r\n",__func__,__LINE__,a) 
#define DEBUG_INFO(fmt, args...) printk(KERN_DEBUG "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_LINE(a)
#define DEBUG_INFO(fmt, args...)
#endif

static struct spi_master *spi0_controller;

struct s3c_spi_info {
    int irq;
    volatile void __iomem *reg_base;
    struct completion done;
    struct spi_transfer *cur_t;
    int cur_cnt;
    struct platform_device *pdev;
};

static int s3c2416_spi_setup(struct spi_device *spi)
{
    int cpol = 0;
    int cpha = 0;
    unsigned char div  = 0;
    unsigned int chcfg = 0;
    unsigned int clkcfg= 0;
    unsigned int modcfg= 0;
    unsigned int inten= 0;
    unsigned int packet = 0;

    struct s3c_spi_info *info;
    struct clk *clk;

    DEBUG_INFO("spi->chip_select: %d",spi->chip_select);

    clk = clk_get(NULL, "pclk");

    info = spi_master_get_devdata(spi->master);
    if (spi->mode & 0x1)
	cpha = 1;
    if (spi->mode & 0x2)
	cpol = 1;
    /* write ch_cfg 0x52000000	: default
     * [6] : 1 high speed	: 1
     * [5] : 0 reset inactive	: 0
     * [4] : 0 -->master	: 0
     * [3] : cpol		: 0
     * [2] : cpha		: 0
     * [1] : 1 -->rx channel on	: 0
     * [0] : 1 -->tx charnnl on	: 0
     */
    chcfg |= (cpol<<3) | (cpha<<2);
    writel(chcfg, info->reg_base + S3C2416_SPI_CHCFG);
    DEBUG_INFO("chcfg: 0x%X",chcfg);

    /* write clk_cfg 0x52000004 
     * [10:9] : 00=pclk; 01=usbclk; 10=epllclock; 11=reserved;
     * [8]    : 1 --> clock enable
     * [7:0]  : prescaler value
     * clock-out = Clock source / ( 2 x (Prescaler value +1))
     * prescaler = PCLK/(CLK*2) - 1	(0~255)
     */
    div = (unsigned char)(DIV_ROUND_UP(clk_get_rate(clk), spi->max_speed_hz * 2) - 1); 
    //clk_put(clk);
    clkcfg |= (1 << 8) | div;
    writel(clkcfg, info->reg_base + S3C2416_SPI_CLKCFG);
    DEBUG_INFO("2416spi: clkcfg = 0x%X, spi_clock = %d, max_speed = %d, prescaler = %d", clkcfg,(unsigned int)clk_get_rate(clk),spi->max_speed_hz,div);

    /* write mode_cfg 0x52000008
     * [30:29] : ch_tran_size 00=byte
     * [28:19] : trailing count
     * [18:17] : bus_tran_size 00=byte
     * [16:11] : rx trigger level
     * [10:5]  : tx trigger level
     * [2]     : 1 --> dma rx on
     * [1]     : 1 --> dma tx on
     * [0]     : dma transfer type, 0=single, 1=4 burst
     */
    modcfg |= (1 << 19) | (1 << 11) | (1 << 5) ;
    writel(modcfg, info->reg_base + S3C2416_SPI_MODECFG);
    DEBUG_INFO("modcfg: 0x%X",modcfg);
/*
    // read hs_spi_status
    status = readl(info->reg_base + S3C2416_SPI_SPISTATUS);
    DEBUG_INFO("spi status: 0x%X",status);
    return 1;
*/
    /* write hs_spi_int_en 0x52000010
     * [6] : IntEnTrailing	1 --> Enable
     * [5] : IntEnRxOverrun
     * [4] : IntEnRxUnderrun
     * [3] : IntEnTxOverrun
     * [2] : IntEnTxUnderrun
     * [1] : IntEnRxFifoRdy
     * [0] : IntEnTxFifoRdy
     */
    inten = (1 << 1) | (1 << 0);
//    inten = 0;
    writel(inten, info->reg_base + S3C2416_SPI_SPIINTEN);
//    writel(0x1f, info->reg_base + S3C2416_SPI_PENDINGCLR);
    DEBUG_INFO("inten: 0x%X",inten);

    /* write packet_count_reg 0x52000020
     * [16]   : 1 --> enable
     * [15:0] : count value
     */
    packet = (1 << 16) | 0xffff;
    writel(packet, info->reg_base + S3C2416_SPI_PACKETCNT);

    /* open tx and rx channel */
    chcfg = readl(info->reg_base + S3C2416_SPI_CHCFG);
    chcfg |= (1 << 1) | (1 << 0);
    writel(chcfg, info->reg_base + S3C2416_SPI_CHCFG);

    return 0;
}

static void set_cs(struct s3c_spi_info *info,int flag)
{
    unsigned int reg = 0;
    unsigned int led = 0;

    reg = readl(info->reg_base + S3C2416_SPI_SLAVESEL);

    /* write slave_selection_reg 0x5200000c default:0x1
     * [9:4] : nCS_time_count
     * [3:2] : reserved
     * [1]   : 0 --> Manual CS; 1 --> Auto CS
     * [0]   : 0 --> nSSout active; 1 --> Inactive
     */
    if (flag == 1){
	led = 0;
	reg |= (1 << 0);
    }
    if (flag == 0){
	led = 1;
	reg &= ~(1 << 0);
    }

    DEBUG_INFO("slave select register: %d, led value: %d", reg, led);
    writel(reg, info->reg_base + S3C2416_SPI_SLAVESEL);
    s3c2410_gpio_setpin(S3C2410_GPC(0),led);
}

static int s3c2416_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
    struct spi_master *master = spi->master;
    struct s3c_spi_info *info;
    struct spi_transfer *t = NULL;

    info = spi_master_get_devdata(master);

//  s3c2410_gpio_setpin(spi->chip_select, 0);
    set_cs(info, 0);

    master->setup(spi);
    DEBUG_INFO("spi->chip_select: %d",spi->chip_select);
    DEBUG_INFO("S3C2410_GPL13: %d",S3C2410_GPL(13));

    list_for_each_entry (t, &mesg->transfers, transfer_list) {
	info->cur_t = t;
	info->cur_cnt = 0;
	init_completion(&info->done);

	if (t->tx_buf){
	    writel(((unsigned char *)t->tx_buf)[0], info->reg_base + S3C2416_SPI_SPITXDATA);
	    DEBUG_INFO("tx_buf: 0x%X, tx_len: %d", ((unsigned char *)t->tx_buf)[0], (unsigned char)t->len);
    DEBUG_LINE(1);
	    // this operation will trigger the spi interrupt,
	    // then finish the rest job in int handler
	    wait_for_completion(&info->done);
	}
	else if (t->rx_buf){
	    writel(0xff, info->reg_base + S3C2416_SPI_SPITXDATA);
    DEBUG_LINE(2);
	    // to trigger interrupt and start recive process
	    wait_for_completion(&info->done);
	}
    }
    
    DEBUG_LINE(3);
    mesg->status = 0;
    DEBUG_LINE(4);
    mesg->complete(mesg->context);

    DEBUG_LINE(5);
//  s3c2410_gpio_setpin(spi->chip_select, 1);
    set_cs(info, 1);

    DEBUG_LINE(6);
    return 0;
}

static irqreturn_t s3c2416_spi_irq(int irq, void *dev_id)
{
    unsigned int status;
    unsigned int inten;

    struct spi_master *master = (struct spi_master*)dev_id;
    struct s3c_spi_info *info = spi_master_get_devdata(master);
    struct spi_transfer *t = info->cur_t;

//    ClearPending(1<<22);

    inten = readl(info->reg_base + S3C2416_SPI_CHCFG);
    writel((1 << 1) | (1 << 0), info->reg_base + S3C2416_SPI_CHCFG);

     // read hs_spi_status
    for (status = 0; status < 12; status++){
	if (((status * 4) == 0x18) || ((status * 4) == 0x1c))
	    continue;
	DEBUG_INFO("register 0x%X: 0x%X",status * 4, readl(info->reg_base + status * 4));
    }

    if (!t){
	writel(inten, info->reg_base + S3C2416_SPI_CHCFG);
	return IRQ_HANDLED;
    }

    DEBUG_LINE(7);
    if (t->tx_buf){
    DEBUG_LINE(8);
	info->cur_cnt++;
	if (info->cur_cnt < t->len)
	    writel(((unsigned char *)t->tx_buf)[info->cur_cnt],info->reg_base + S3C2416_SPI_SPITXDATA);
	else
	    complete(&info->done);
    }else {
    DEBUG_LINE(9);
	((unsigned char *)t->rx_buf)[info->cur_cnt] = readl(info->reg_base + S3C2416_SPI_SPIRXDATA);
	info->cur_cnt++;
	
	if (info->cur_cnt < t->len)
	    writel(0xff, info->reg_base + S3C2416_SPI_SPITXDATA);
	else
	    complete(&info->done);
    }

    writel(inten, info->reg_base + S3C2416_SPI_CHCFG);
    return IRQ_HANDLED;
}

static int s3c2416_spi_gpio_init(int bus_num)
{
    struct clk *clk = clk_get(NULL, "spi");

    if (IS_ERR(clk)){
	printk("s3c2416 spi master: Failed to get clock");
	return -1;
    }
    clk_enable(clk);
    clk_put(clk);

    /* init gpio 
     * GPL13 --> CS
     * GPE11 --> MISO
     * GPE12 --> MOSI
     * GPE13 --> CLK
     */
    if (bus_num == 0){
	s3c2410_gpio_cfgpin(S3C2410_GPL(13), S3C2410_GPL13_SPISS0);
	s3c2410_gpio_cfgpin(S3C2410_GPE(11), S3C2410_GPE11_SPIMISO0);
	s3c2410_gpio_cfgpin(S3C2410_GPE(12), S3C2410_GPE12_SPIMOSI0);
	s3c2410_gpio_cfgpin(S3C2410_GPE(13), S3C2410_GPE13_SPICLK0);
	s3c2410_gpio_cfgpin(S3C2410_GPC(0),  S3C2410_GPIO_OUTPUT);
    }

    return 0;
}

static struct spi_master * create_spi_master(int bus_num, unsigned int reg_base_phy, int irq)
{
    int ret;
    struct spi_master *master;
    struct s3c_spi_info *info;
    struct platform_device *pdev;

    pdev = platform_device_alloc("s3c2410_spi", bus_num);
    platform_device_add(pdev);


    ret = s3c2416_spi_gpio_init(bus_num);   // gpio initial
    if (ret){
	printk("s3c2416 spi master: Failed to initial GPIO!\n");
	return NULL;
    }

    master = spi_alloc_master(&pdev->dev, sizeof(struct s3c_spi_info));
    master->bus_num = bus_num;
    master->num_chipselect = 0xffff;
    master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

    master->setup = s3c2416_spi_setup;
    master->transfer = s3c2416_spi_transfer;

    info = spi_master_get_devdata(master);
    info->reg_base = (volatile void *)ioremap(reg_base_phy, 0x30);
    info->irq = irq;
    info->pdev = pdev;

    ret = request_irq(irq, s3c2416_spi_irq, 0, "s3c2410_spi", master);
    if (ret){
	printk("s3c2416 spi master: Failed to request irq!\n");
	return NULL;
    }
    DEBUG_INFO("registed irq : %d", irq);

    ret = spi_register_master(master);
    if (ret){
	printk("s3c2416 spi master: Failed to register master!\n");
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
    free_irq(info->irq, master);
    iounmap((void *)info->reg_base);
//    kfree(master);
}

static int s3c2416_spi_init(void)
{
    spi0_controller = create_spi_master(0, S3C2416_SPI_BASE, IRQ_SPI0);
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
