/* linux/arch/arm/mach-s3c2416/mach-hanlin_v3c.c
 *
 * Copyright (c) 2009 Yauhen Kharuzhy <jekhor@gmail.com>,
 *	as part of OpenInkpot project
 * Copyright (c) 2009 Promwad Innovation Company
 *	Yauhen Kharuzhy <yauhen.kharuzhy@promwad.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/mtd/partitions.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>

#include <mach/idle.h>
#include <mach/leds-gpio.h>
#include <plat/iic.h>

#include <plat/s3c2416.h>
#include <plat/gpio-cfg.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/nand.h>
#include <plat/sdhci.h>
#include <plat/mci.h>

#include <plat/regs-fb-v4.h>
#include <plat/fb.h>
//#include <plat/ts.h>
#include <plat/ts1.h>
#include <plat/common-smdk.h>
#include <linux/smsc911x.h>

#include <linux/platform_data/s3c-hsudc.h>

static struct map_desc smdk2416_iodesc[] __initdata = {
	/* ISA IO Space map (memory space selected by A24) */

	{
		.virtual	= (u32)S3C24XX_VA_ISA_WORD,
		.pfn		= __phys_to_pfn(S3C2410_CS2),
		.length		= 0x10000,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_WORD + 0x10000,
		.pfn		= __phys_to_pfn(S3C2410_CS2 + (1<<24)),
		.length		= SZ_4M,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_BYTE,
		.pfn		= __phys_to_pfn(S3C2410_CS2),
		.length		= 0x10000,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_BYTE + 0x10000,
		.pfn		= __phys_to_pfn(S3C2410_CS2 + (1<<24)),
		.length		= SZ_4M,
		.type		= MT_DEVICE,
	}
};

#define UCON (S3C2410_UCON_DEFAULT	| \
		S3C2440_UCON_PCLK	| \
		S3C2443_UCON_RXERR_IRQEN)

#define ULCON (S3C2410_LCON_CS8 | S3C2410_LCON_PNONE)

#define UFCON (S3C2410_UFCON_RXTRIG8	| \
		S3C2410_UFCON_FIFOMODE	| \
		S3C2440_UFCON_TXTRIG16)

static struct s3c2410_uartcfg smdk2416_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	/* IR port */
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON /*| 0x50*/,
		.ufcon	     = UFCON,
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	}
};

static u64 samsung_device_dma_mask = DMA_BIT_MASK(32);
 #define S3C2416_PA_HSUDC  (0x49800000)
 #define S3C2416_SZ_HSUDC  (SZ_4K)

 #ifdef CONFIG_PLAT_S3C24XX
 static struct resource s3c_hsudc_resource[] = {
  [0] = DEFINE_RES_MEM(S3C2416_PA_HSUDC, S3C2416_SZ_HSUDC),
  [1] = DEFINE_RES_IRQ(IRQ_USBD),
};
 
struct platform_device s3c_device_usb_hsudc = {
   .name   = "s3c-hsudc",
   .id   = -1,
   .num_resources  = ARRAY_SIZE(s3c_hsudc_resource),
   .resource = s3c_hsudc_resource,
   .dev    = {
   .dma_mask   = &samsung_device_dma_mask,
   .coherent_dma_mask  = DMA_BIT_MASK(32),
  },
};
  
void __init s3c24xx_hsudc_set_platdata(struct s3c24xx_hsudc_platdata *pd)
{ 
  s3c_set_platdata(pd, sizeof(*pd), &s3c_device_usb_hsudc);
 }
#endif /* CONFIG_PLAT_S3C24XX */


static void smdk2416_hsudc_gpio_init(void)
{
  s3c_gpio_setpull(S3C2410_GPH(14), S3C_GPIO_PULL_UP);
  s3c_gpio_setpull(S3C2410_GPF(2), S3C_GPIO_PULL_NONE);
  s3c_gpio_cfgpin(S3C2410_GPH(14), S3C_GPIO_SFN(1));
  s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 0);
}

static void smdk2416_hsudc_gpio_uninit(void)
{
 s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 1);
 s3c_gpio_setpull(S3C2410_GPH(14), S3C_GPIO_PULL_NONE);
 s3c_gpio_cfgpin(S3C2410_GPH(14), S3C_GPIO_SFN(0));
}
 
static struct s3c24xx_hsudc_platdata smdk2416_hsudc_platdata = {
   .epnum = 9,
  .gpio_init = smdk2416_hsudc_gpio_init,
  .gpio_uninit = smdk2416_hsudc_gpio_uninit,
};


struct s3c_fb_pd_win smdk2416_fb_win[] = {
	[0] = {
		/* think this is the same as the smdk6410 */
		.win_mode	= {
			.pixclock	= 56553,
			.left_margin	= 40,//20  (4寸)
			.right_margin	= 40,//68
			.upper_margin	= 10,//18
			.lower_margin	= 35,//17
			.hsync_len	= 22,//31
			.vsync_len	= 2,//5
			.xres           = 800,//480
			.yres           = 480,//272
		},
		.default_bpp	= 16,
		.max_bpp	= 32,
	},
};

static void s3c2416_fb_gpio_setup_24bpp(void)
{
	unsigned int gpio;

	for (gpio = S3C2410_GPC(1); gpio <= S3C2410_GPC(4); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	for (gpio = S3C2410_GPC(8); gpio <= S3C2410_GPC(15); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	for (gpio = S3C2410_GPD(0); gpio <= S3C2410_GPD(15); gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

static struct s3c_fb_platdata smdk2416_fb_platdata = {
	.win[0]		= &smdk2416_fb_win[0],
	.setup_gpio	= s3c2416_fb_gpio_setup_24bpp,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
};




/*   SD    */

static struct s3c_sdhci_platdata smdk2416_hsmmc0_pdata __initdata = {
	.max_width		= 4,
	.cd_type		= S3C_SDHCI_CD_NONE,
	.host_caps              = ( MMC_CAP_4_BIT_DATA |
                                MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
                                MMC_CAP_DISABLE),
//        .cd_type                = S3C_SDHCI_CD_PERMANENT,
};

static struct s3c_sdhci_platdata smdk2416_hsmmc1_pdata __initdata = {
	.max_width		= 4,
	.cd_type		= S3C_SDHCI_CD_NONE,
};


/* SMC9115 LAN via ROM interface */

static struct resource s3c_smc911x_resources[] = {
      [0] = {
              .start  = 0x08000000,
              .end    = 0x08000000 + SZ_16M - 1,
              .flags  = IORESOURCE_MEM,
      },
      [1] = {

		  .start = IRQ_EINT(4), //beelike change!
		  .end   = IRQ_EINT(4),

		  .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_smc911x = {
      .name           = "smc911x",
      .id             =  -1,
      .num_resources  = ARRAY_SIZE(s3c_smc911x_resources),
      .resource       = s3c_smc911x_resources,
};

struct smsc911x_platform_config s3c_smsc911x_config = {
       .irq_polarity   =  SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
       .irq_type   		 =  SMSC911X_IRQ_TYPE_OPEN_DRAIN,
       .flags          =  SMSC911X_USE_16BIT | SMSC911X_FORCE_INTERNAL_PHY,
			 .phy_interface  =  PHY_INTERFACE_MODE_MII,
};

struct platform_device s3c_device_smsc911x =
{
	.name	= "smsc911x",
	.id		= -1,
	.num_resources = ARRAY_SIZE(s3c_smc911x_resources),
	.resource			= s3c_smc911x_resources,
	.dev					= 
				{
					.platform_data = &s3c_smsc911x_config,
			  },
};



/*i2c device  */

static struct  i2c_board_info smdk2416_i2c_dev __initdata  = {
	I2C_BOARD_INFO("wm8731",0x1a),
};

/*spi device for COM2416 */
static struct spi_board_info spi_info_com2416[] = {
	{
	 .modalias = "oled",
	 .max_speed_hz = 10000000,
	 .bus_num = 0
	 .mode = SPI_MODE_0,
	 .chip_select = 0,
	}
}

/* Touch srcreen */
static struct resource s3c_ts_resource[] = {
	[0] = {
		.start = S3C24XX_PA_ADC,
		.end   = S3C24XX_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TC,
		.end   = IRQ_TC,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}
	
};

struct platform_device s3c_device_ts_1 = {
	.name		  = "s3c-ts",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_ts_resource),
	.resource	  = s3c_ts_resource,
};

 void __init s3c_ts_set_platdata1(struct s3c_ts_mach_info *pd)
{
	struct s3c_ts_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_ts_1.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for Touchscreen platform data\n");
	}
}

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 5,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_1, //s3c2416
};

/*   sdi 

static struct resource s3c_sdi_resource[] = {
        [0] = {
                .start = 0x4ac00000,
                .end   = 0x4ac00000 + 0x1000 - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_S3C2416_HSMMC0,
                .end   = IRQ_S3C2416_HSMMC0,
                .flags = IORESOURCE_IRQ,
        }

};

struct platform_device s3c_device_sdi_1 = {
        .name             = "s3c2416-sdi",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_sdi_resource),
        .resource         = s3c_sdi_resource,
};

*/






/****************************  device init  table  ******************/ 

static struct platform_device *smdk2416_devices[] __initdata = {
	&s3c_device_fb,
	&s3c_device_wdt,
	&s3c_device_ohci,
	&s3c_device_i2c0,
//	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_rtc,
//	&s3c_device_smc911x,
	&s3c_device_smsc911x,
//	&s3c_device_adc,
	&s3c_device_ts_1,
        &samsung_asoc_dma,	
	&s3c_device_iis,
//	&s3c_device_sdi_1,
	&s3c_device_usb_hsudc,
	&s3c_device_spi0,
};

static void __init smdk2416_map_io(void)
{
	s3c24xx_init_io(smdk2416_iodesc, ARRAY_SIZE(smdk2416_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(smdk2416_uartcfgs, ARRAY_SIZE(smdk2416_uartcfgs));
}



static void __init smdk2416_machine_init(void)
{


	s3c_i2c0_set_platdata(NULL);
	s3c_fb_set_platdata(&smdk2416_fb_platdata);

	s3c_sdhci0_set_platdata(&smdk2416_hsmmc0_pdata);
	s3c_sdhci1_set_platdata(&smdk2416_hsmmc1_pdata);
	//printk("before s3c24xx_ts_set_platdata..\n");
	s3c_ts_set_platdata1(&s3c_ts_platform);
	//printk("after s3c24xx_ts_set_platdata..\n");

	s3c24xx_hsudc_set_platdata(&smdk2416_hsudc_platdata);

	gpio_request(S3C2410_GPB(4), "USBHost Power");
	gpio_direction_output(S3C2410_GPB(4), 1);

	gpio_request(S3C2410_GPB(3), "Display Power");
	gpio_direction_output(S3C2410_GPB(3), 1);

	gpio_request(S3C2410_GPB(1), "Display Reset");
	gpio_direction_output(S3C2410_GPB(1), 1);


	i2c_register_board_info(0,&smdk2416_i2c_dev,1);
	
	spi_register_board_info(spi_info_com2416,ARRAY_SIZE(spi_info_com2416));

	platform_add_devices(smdk2416_devices, ARRAY_SIZE(smdk2416_devices));
	smdk_machine_init();
}

MACHINE_START(SMDK2416, "SMDK2416")
	/* Maintainer: Yauhen Kharuzhy <jekhor@gmail.com> */
	.boot_params	= S3C2410_SDRAM_PA + 0x100,

	.init_irq	= s3c24xx_init_irq,
	.map_io		= smdk2416_map_io,
	.init_machine	= smdk2416_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END
