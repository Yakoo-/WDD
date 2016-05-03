#include <mach/map.h>
#include <mach/regs-irq.h>
#include <mach/regs-gpio.h>
#include <mach/irqs.h>

#define DEBUG	1
/*
 * buffer size = 1024 * 4 Byte
 * CCD resolution = 10
 * Datain pin = GPG4
 */
#define CCD_RES			10
#define CCD_MAX_PIXEL_CNT	1024
#define CCD_BUFFER_SIZE		FIQ_BUFFER_SIZE
#define CCD_RES_MASK		0x3ff	// 10bit
// start pulse low time is 3 pixel cycles, about 34+12*3=70 clock cycles of CCD,refer to S10077 USER MANUAL
#define CCD_START_TLP		3	

#define DATA_GPIO_BANK		GPG
#define DATA_PIN_NUM		4
#define DATA_PIN_MASK		(1 << DATA_PIN_NUM)
#define DATA_GPXDAT_BASE	S3C2410_GPGDAT

#ifdef DEBUG
#define BUZ_GPIO_BANK		GPF
#define BUZ_PIN_NUM		0
#define BUZ_PIN_MASK		(1 << BUZ_PIN_NUM)
#define	BUZ_GPXDAT_BASE		S3C2410_GPFDAT
#endif

#define IRQ_NUM			IRQ_EINT3
#define	FIQ_ACK_BIT		(1 << (IRQ_NUM - IRQ_EINT0))
#define FIQ_BUFFER_LENGTH	CCD_MAX_PIXEL_CNT
#define FIQ_BUFFER_CELL_SIZE	(sizeof(unsigned int))
#define FIQ_BUFFER_SIZE		(FIQ_BUFFER_CELL_SIZE * FIQ_BUFFER_LENGTH)

#define FIQ_STATUS_STOPPED	0
#define FIQ_STATUS_RUNNING	1
#define FIQ_STATUS_FINISHED	2
#define FIQ_STATUS_ERR		3

#ifdef __ASSEMBLY__
#define __REG_NR(x)     r##x
#else
#define __REG_NR(x)     (x)
#endif

#define reg_args	__REG_NR(8)	    
#define reg_buffer	__REG_NR(9)
#define reg_tmpd	__REG_NR(10)	// temp register for data
#define reg_tmpa	__REG_NR(11)	// temp register for address
#define reg_index	__REG_NR(12)
#define reg_datain	__REG_NR(13)

