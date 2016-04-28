#define DEBUG	1

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#include <linux/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/spi/Font_Libs.h>

#define OLED_CMD_INIT	    0x01
#define OLED_CMD_CLEAR_ALL  0x02
#define OLED_CMD_CLEAR_PAGE 0x03
#define OLED_CMD_FILL	    0x04
#define OLED_CMD_SET_POS    0x05    //Y=page= (arg >> 8) & 0xff; X=col= arg & 0xff;
#define OLED_CMD_WR_DAT	    0x06
#define OLED_CMD_WR_CMD	    0x07
#define uchar8	    unsigned char
#define uchar	    unsigned char
#define ushort16    unsigned short
#define ushort    unsigned short

#define DEVICE_NAME	"oled"

#ifdef DEBUG
#define DEBUG_LINE(a) 	printk(KERN_DEBUG "[%s:%d] flag=%d\r\n",__func__,__LINE__,a) 
#define DEBUG_INFO(fmt, args...) printk(KERN_DEBUG "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_LINE(a)
#define DEBUG_INFO(fmt, args...)
#endif

static int major;
static struct class *class;
static int spi_oled_res;
static int spi_oled_dc;
static struct spi_device *spi_oled_dev;
static uchar8 *ker_buf;

//OLED写数据
void OLED_WrDat(uchar8 data)
{
    s3c2410_gpio_setpin(spi_oled_dc, 1);
    spi_write(spi_oled_dev, &data, 1);

}

//OLED写命令
void OLED_WrCmd(uchar8 cmd)
{
    s3c2410_gpio_setpin(spi_oled_dc, 0);
    spi_write(spi_oled_dev, &cmd, 1);
}


void SetStartColumn(uchar8 ucData)
{
    OLED_WrCmd(0x00+ucData % 16);   // Set Lower Column Start Address for Page Addressing Mode
                                   // Default => 0x00
    OLED_WrCmd(0x10+ucData / 16);   // Set Higher Column Start Address for Page Addressing Mode
                                   // Default => 0x10
}

void SetAddressingMode(uchar8 ucData)
{
    OLED_WrCmd(0x20);        // Set Memory Addressing Mode
    OLED_WrCmd(ucData);      // Default => 0x02
                            // 0x00 => Horizontal Addressing Mode
                            // 0x01 => Vertical Addressing Mode
                            // 0x02 => Page Addressing Mode
}

void SetColumnAddress(uchar8 a, uchar8 b)
{
    OLED_WrCmd(0x21);        // Set Column Address
    OLED_WrCmd(a);           // Default => 0x00 (Column Start Address)
    OLED_WrCmd(b);           // Default => 0x7F (Column End Address)
}

void SetPageAddress(uchar8 a, uchar8 b)
{
    OLED_WrCmd(0x22);        // Set Page Address
    OLED_WrCmd(a);           // Default => 0x00 (Page Start Address)
    OLED_WrCmd(b);           // Default => 0x07 (Page End Address)
}

void SetStartLine(uchar8 ucData)
{
    OLED_WrCmd(0x40|ucData); // Set Display Start Line
                            // Default => 0x40 (0x00)
}

void SetContrastControl(uchar8 ucData)
{
    OLED_WrCmd(0x81);        // Set Contrast Control
    OLED_WrCmd(ucData);      // Default => 0x7F
}

void SetChargePump(uchar8 ucData)
{
    OLED_WrCmd(0x8D);        // Set Charge Pump
    OLED_WrCmd(0x10|ucData); // Default => 0x10
                            // 0x10 (0x00) => Disable Charge Pump
                            // 0x14 (0x04) => Enable Charge Pump
}

void SetSegmentRemap(uchar8 ucData)
{
    OLED_WrCmd(0xA0|ucData); // Set Segment Re-Map
                            // Default => 0xA0
                            // 0xA0 (0x00) => Column Address 0 Mapped to SEG0
                            // 0xA1 (0x01) => Column Address 0 Mapped to SEG127
}

void SetEntireDisplay(uchar8 ucData)
{
    OLED_WrCmd(0xA4|ucData); // Set Entire Display On / Off
                            // Default => 0xA4
                            // 0xA4 (0x00) => Normal Display
                            // 0xA5 (0x01) => Entire Display On
}

void SetInverseDisplay(uchar8 ucData)
{
    OLED_WrCmd(0xA6|ucData); // Set Inverse Display On/Off
                            // Default => 0xA6
                            // 0xA6 (0x00) => Normal Display
                            // 0xA7 (0x01) => Inverse Display On
}

void SetMultiplexRatio(uchar8 ucData)
{
    OLED_WrCmd(0xA8);        // Set Multiplex Ratio
    OLED_WrCmd(ucData);      // Default => 0x3F (1/64 Duty)
}

void SetDisplayOnOff(uchar8 ucData)
{
    OLED_WrCmd(0xAE|ucData); // Set Display On/Off
                            // Default => 0xAE
                            // 0xAE (0x00) => Display Off
                            // 0xAF (0x01) => Display On
}

void SetStartPage(uchar8 ucData)
{
    OLED_WrCmd(0xB0|ucData); // Set Page Start Address for Page Addressing Mode
                            // Default => 0xB0 (0x00)
}

void SetCommonRemap(uchar8 ucData)
{
    OLED_WrCmd(0xC0|ucData); // Set COM Output Scan Direction
                            // Default => 0xC0
                            // 0xC0 (0x00) => Scan from COM0 to 63
                            // 0xC8 (0x08) => Scan from COM63 to 0
}

void SetDisplayOffset(uchar8 ucData)
{
    OLED_WrCmd(0xD3);        // Set Display Offset
    OLED_WrCmd(ucData);      // Default => 0x00
}

void SetDisplayClock(uchar8 ucData)
{
    OLED_WrCmd(0xD5);        // Set Display Clock Divide Ratio / Oscillator Frequency
    OLED_WrCmd(ucData);      // Default => 0x80
                            // D[3:0] => Display Clock Divider
                            // D[7:4] => Oscillator Frequency
}

void SetPrechargePeriod(uchar8 ucData)
{
    OLED_WrCmd(0xD9);        // Set Pre-Charge Period
    OLED_WrCmd(ucData);      // Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
                            // D[3:0] => Phase 1 Period in 1~15 Display Clocks
                            // D[7:4] => Phase 2 Period in 1~15 Display Clocks
}

void SetCommonConfig(uchar8 ucData)
{
    OLED_WrCmd(0xDA);        // Set COM Pins Hardware Configuration
    OLED_WrCmd(0x02|ucData); // Default => 0x12 (0x10)
                            // Alternative COM Pin Configuration
                            // Disable COM Left/Right Re-Map
}

void SetVCOMH(uchar8 ucData)
{
    OLED_WrCmd(0xDB);        // Set VCOMH Deselect Level
    OLED_WrCmd(ucData);      // Default => 0x20 (0.77*VCC)
}

/*****************************************************************************
 函 数 名  : OLED_SetPos
 功能描述  : 设置坐标
 输入参数  : uchar8 ucIdxX
             uchar8 ucIdxY
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_SetPos(uchar8 ucIdxX, uchar8 ucIdxY)
{
    OLED_WrCmd(0xb0 + ucIdxY);
    OLED_WrCmd(((ucIdxX & 0xf0) >> 4) | 0x10);
    OLED_WrCmd((ucIdxX & 0x0f) | 0x00);
}

/*****************************************************************************
 函 数 名  : OLED_Fill
 功能描述  : 对全屏写入同一个字符函数
             如 OLED_Fill(0x01);    对于某一位0为不亮 1为亮
            ------------------------------------------------------
 输入参数  : uchar8 ucData
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_Fill(uchar8 ucData)
{
    unsigned int ucPage,ucColumn;

    for(ucPage = 0; ucPage < 8; ucPage++)
    {
        OLED_WrCmd(0xb0 + ucPage);  //0xb0+0~7表示页0~7
        OLED_WrCmd(0x00);           //0x00+0~16表示将128列分成16组其地址在某组中的第几列
        OLED_WrCmd(0x10);           //0x10+0~16表示将128列分成16组其地址所在第几组
        for(ucColumn = 0; ucColumn < 128; ucColumn++)
        {
            OLED_WrDat(ucData);
        }
    }
}

/*********************************
 * Func name: OLED_ClearPage
 * Desc	    : clear 1 page (from 0 ~ 7)
 ********************************/
void OLED_ClearPage(uchar8 ucPage)
{
    uchar8 ucColumn;
    OLED_WrCmd(0xb0 + ucPage);  //0xb0+0~7表示页0~7
    OLED_WrCmd(0x00);           //0x00+0~16表示将128列分成16组其地址在某组中的第几列
    OLED_WrCmd(0x10);           //0x10+0~16表示将128列分成16组其地址所在第几组
    for(ucColumn = 0; ucColumn < 128; ucColumn++)
    {
	OLED_WrDat(0);
    }
}


/*****************************************************************************
 函 数 名  : OLED_Init
 功能描述  : OOLED初始化
 输入参数  : light == 1: light all the screen 
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_Init(unsigned char light)
{
    unsigned int i;

    s3c2410_gpio_setpin(spi_oled_res, 0);
    for(i = 0; i < 1000; i++)
    {
        asm("nop");            //从上电到下面开始初始化要有足够的时间，即等待RC复位完毕
    }
    s3c2410_gpio_setpin(spi_oled_res, 1);

    SetDisplayOnOff(0x00);     // Display Off (0x00/0x01)
    SetDisplayClock(0x80);     // Set Clock as 100 Frames/Sec
    SetMultiplexRatio(0x3F);   // 1/64 Duty (0x0F~0x3F)
    SetDisplayOffset(0x00);    // Shift Mapping RAM Counter (0x00~0x3F)
    SetStartLine(0x00);        // Set Mapping RAM Display Start Line (0x00~0x3F)
    SetChargePump(0x04);       // Enable Embedded DC/DC Converter (0x00/0x04)
    SetAddressingMode(0x02);   // Set Page Addressing Mode (0x00/0x01/0x02)
    SetSegmentRemap(0x01);     // Set SEG/Column Mapping     0x00左右反置 0x01正常
    SetCommonRemap(0x08);      // Set COM/Row Scan Direction 0x00上下反置 0x08正常
    SetCommonConfig(0x10);     // Set Sequential Configuration (0x00/0x10)
    SetContrastControl(0xCF);  // Set SEG Output Current
    SetPrechargePeriod(0xF1);  // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    SetVCOMH(0x40);            // Set VCOM Deselect Level
    SetEntireDisplay(0x00);    // Disable Entire Display On (0x00/0x01)
    SetInverseDisplay(0x00);   // Disable Inverse Display On (0x00/0x01)
    SetDisplayOnOff(0x01);     // Display On (0x00/0x01)
    (light & 0x1) ? OLED_Fill(0xFF) : OLED_Fill(0x00) ;	 // 初始清屏
    OLED_SetPos(0,0);

    return;
}

static long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned char X, Y;    
    switch (cmd)
    {
	case OLED_CMD_INIT:
	    OLED_Init(arg & 0x01);
	    return 0;
	case OLED_CMD_CLEAR_ALL:
	    OLED_Fill(0x00);
	    return 0;
	case OLED_CMD_CLEAR_PAGE:
	    if (arg < 8){
		OLED_ClearPage((char)arg);
		return 0;
	    }else{
		return -EINVAL;
	    }
	case OLED_CMD_FILL:
	    OLED_Fill((uchar)arg);
	    return 0;
	case OLED_CMD_SET_POS:
	    X= (uchar)(arg & 0xff);		// index X
	    Y= (uchar)((arg >> 8) & 0xff);	// index Y
	    OLED_SetPos(X, Y);
	    return 0;
	case OLED_CMD_WR_DAT:
	    OLED_WrDat((uchar)arg);
	    return 0;
	case OLED_CMD_WR_CMD:
	    OLED_WrCmd((uchar)arg);
	    return 0;
	default:
	    return -EINVAL;
    }
}

static ssize_t oled_write(struct file *file, const uchar8 __user *buf, size_t count, loff_t *ppos)
{
    int err;

    if (count > 4096)
	return -EINVAL;
    err = copy_from_user(ker_buf, buf, count);
    if(err){
	printk(KERN_ERR "Failed to copy data from user!");
	return err;
    }

    spi_write(spi_oled_dev, buf,count);
    return 0;
}


static struct file_operations oled_ops = {
    .owner	    = THIS_MODULE,
    .write	    = oled_write,
    .unlocked_ioctl = oled_ioctl,
};

static int __devinit spi_oled_probe(struct spi_device *spi)
{
    int *platdata;

    spi_oled_dev = spi;

    platdata = (int *)(spi->dev.platform_data);
    if (platdata == NULL){
	printk("ERROR: Failed to get platform data!\n");
	return -1;
    } else {
	spi_oled_res = platdata[0];
	spi_oled_dc  = platdata[1];
    }
    s3c2410_gpio_cfgpin(spi_oled_res, S3C2410_GPIO_OUTPUT);
    s3c2410_gpio_cfgpin(spi_oled_dc,  S3C2410_GPIO_OUTPUT);
    s3c2410_gpio_setpin(spi_oled_res, 1);
    s3c2410_gpio_setpin(spi_oled_dc, 1);

    ker_buf = kmalloc(4096, GFP_KERNEL);
    
    // regist a file operation
    major = register_chrdev(0, "oled", &oled_ops);
    // make a device class 
    class = class_create(THIS_MODULE, "oled");
    // make a device node
    device_create(class, NULL, MKDEV(major, 0), NULL, "oled");

    return 0;
}

static int __devexit spi_oled_remove(struct spi_device *spi)
{
    device_destroy(class, MKDEV(major, 0));
    class_destroy(class);
    unregister_chrdev(major, "oled");
    kfree(ker_buf);
    return 0;
}

static struct spi_driver spi_oled_driver = {
    .driver = {
	.name	= "oled",
	.owner	= THIS_MODULE,
    },
    .probe	= spi_oled_probe,
    .remove	= __devexit_p(spi_oled_remove),
};

static int spi_oled_init(void)
{
    return spi_register_driver(&spi_oled_driver);
}

static void spi_oled_exit(void)
{
    spi_unregister_driver(&spi_oled_driver);
}

module_init(spi_oled_init);
module_exit(spi_oled_exit);
MODULE_DESCRIPTION("OLED SPI Driver");
MODULE_LICENSE("GPL");
