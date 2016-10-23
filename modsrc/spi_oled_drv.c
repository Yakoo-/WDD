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

#include "oled.h"

#define OLED_PIN_RES    S3C2410_GPE(14)
#define OLED_PIN_DC     S3C2410_GPE(15)


static int major;
static struct class *class;
static const int spi_oled_res = OLED_PIN_RES;
static const int spi_oled_dc = OLED_PIN_DC;
static struct spi_device *spi_oled_dev;
static uchar8 *ker_buf;

void OLED_WrDat(uchar8 data)
{
    s3c2410_gpio_setpin(spi_oled_dc, 1);
    spi_write(spi_oled_dev, &data, 1);

}

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

void OLED_SetPos(uchar8 ucIdxX, uchar8 ucIdxY)
{
    OLED_WrCmd(0xb0 + ucIdxY);
    OLED_WrCmd(((ucIdxX & 0xf0) >> 4) | 0x10);
    OLED_WrCmd( (ucIdxX & 0x0f) | 0x00);
}

/*********************************
 * Func name: OLED_ClearPage
 * Desc	    : clear 1 page (from 0 ~ 7)
 ********************************/
void OLED_ClearPage(uchar8 ucPage)
{
    uchar8 ucColumn;
    for(ucColumn = 0; ucColumn < 128; ucColumn++)
    {
        OLED_SetPos(ucColumn, ucPage);
        OLED_WrDat(0);
    }
}

void OLED_Fill(uchar8 ucData)
{
    unsigned int ucPage,ucColumn;
    DEBUG_NUM(ucData);

    for(ucPage = 0; ucPage < 8; ucPage++)
    {
        OLED_WrCmd(0xb0 + ucPage); 
        OLED_WrCmd(0x00);
        OLED_WrCmd(0x10);
        for(ucColumn = 0; ucColumn < 128; ucColumn++)
        {
            OLED_WrDat(ucData);
        }
        /* OLED_ClearPage(ucData); */
    }
}
/*****************************************************************************
 �� �� ��  : OLED_Init
 ��������  : OOLED��ʼ��
 �������  : light == 1: light all the screen 
 �������  : NONE
 �� �� ֵ  : NONE
*****************************************************************************/
void OLED_Init(unsigned char light)
{
    unsigned int i;

    s3c2410_gpio_setpin(spi_oled_res, 0);
    for(i = 0; i < 1000; i++)
    {
        asm("nop");            //���ϵ絽���濪ʼ��ʼ��Ҫ���㹻��ʱ�䣬���ȴ�RC��λ���
    }
    s3c2410_gpio_setpin(spi_oled_res, 1);

    SetDisplayOnOff(0x00);     // Display Off (0x00/0x01)
    SetDisplayClock(0x80);     // Set Clock as 100 Frames/Sec
    SetMultiplexRatio(0x3F);   // 1/64 Duty (0x0F~0x3F)
    SetDisplayOffset(0x00);    // Shift Mapping RAM Counter (0x00~0x3F)
    SetStartLine(0x00);        // Set Mapping RAM Display Start Line (0x00~0x3F)
    SetChargePump(0x04);       // Enable Embedded DC/DC Converter (0x00/0x04)
    SetAddressingMode(0x02);   // Set Page Addressing Mode (0x00/0x01/0x02)
    SetSegmentRemap(0x01);     // Set SEG/Column Mapping     0x00���ҷ��� 0x01����
    SetCommonRemap(0x08);      // Set COM/Row Scan Direction 0x00���·��� 0x08����
    SetCommonConfig(0x10);     // Set Sequential Configuration (0x00/0x10)
    SetContrastControl(0xCF);  // Set SEG Output Current
    SetPrechargePeriod(0xF1);  // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    SetVCOMH(0x40);            // Set VCOM Deselect Level
    SetEntireDisplay(0x00);    // Disable Entire Display On (0x00/0x01)
    SetInverseDisplay(0x00);   // Disable Inverse Display On (0x00/0x01)
    SetDisplayOnOff(0x01);     // Display On (0x00/0x01)
    (light & 0x1) ? OLED_Fill(0xFF) : OLED_Fill(0x00) ;	 // ��ʼ����
    OLED_SetPos(0,0);
}

static long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned char X, Y;    
    unsigned char i = 0;

    switch (cmd) {
	case OLED_CMD_INIT:
	    OLED_Init(arg & 0x01);
        break;
	case OLED_CMD_CLR_ALL:
	    OLED_Fill(0x00);
        break;
	case OLED_CMD_CLEAR_ROW:
        OLED_ClearPage( (char)(0xff & arg) );
        break;
	case OLED_CMD_FILL:
	    OLED_Fill((uchar)arg);
        break;
	case OLED_CMD_SET_POS:
	    X= (uchar)(arg & 0xff);		// index X
	    Y= (uchar)((arg >> 8) & 0xff);	// index Y
	    OLED_SetPos(X, Y);
        break;
	case OLED_CMD_WR_DAT:
	    OLED_WrDat((uchar)arg);
        break;
	case OLED_CMD_WR_CMD:
	    OLED_WrCmd((uchar)arg);
        break;
    case OLED_CMD_CLR_EXCEPT:	
        for (i = 0; i < OLED_ROW_NUM; i++)
            if (i == arg)
                continue;
            else
                OLED_ClearPage(i);
        break;
	default:
        break;
    }
    return 0;
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
    spi_oled_dev = spi;

    s3c2410_gpio_cfgpin(spi_oled_res, S3C2410_GPIO_OUTPUT);
    s3c2410_gpio_cfgpin(spi_oled_dc,  S3C2410_GPIO_OUTPUT);
    s3c2410_gpio_setpin(spi_oled_res, 1);
    s3c2410_gpio_setpin(spi_oled_dc, 1);

    ker_buf = kmalloc(4096, GFP_KERNEL);
    
    // regist a file operation
    major = register_chrdev(0, OLED_DEVICE_NAME, &oled_ops);
    // make a device class 
    class = class_create(THIS_MODULE, OLED_DEVICE_NAME);
    // make a device node
    device_create(class, NULL, MKDEV(major, 0), NULL, OLED_DEVICE_NAME);

    return 0;
}

static int __devexit spi_oled_remove(struct spi_device *spi)
{
    device_destroy(class, MKDEV(major, 0));
    class_destroy(class);
    unregister_chrdev(major, OLED_DEVICE_NAME);
    kfree(ker_buf);
    return 0;
}

static struct spi_driver spi_oled_driver = {
    .driver = {
	.name	= OLED_DEVICE_NAME,
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
