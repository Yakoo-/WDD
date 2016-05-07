#include <linux/linkage.h>
#include <asm/assembler.h>

#include <mach/map.h>
#include <mach/regs-gpio.h>
#include <mach/regs-irq.h>

#include "ccd_s3c2416_fiq.h"

#define	ARGS_STATUS_OFFSET      (0 << 2)    // 0 * 4
#define	ARGS_VA_IRQ_OFFSET		(1 << 2)    // 1 * 4
#define	ARGS_VA_GPIO_OFFSET	(2 << 2)    // 2 * 4

    .text

ENTRY(ccd_s3c2416_fiq_handler)
ccd_s3c2416_fiq_handler:
    .word   fiq_end - fiq_start
fiq_start:
    /* clear fiq bit of the SCRPND register */
    mov	    reg_tmpd, # FIQ_ACK_BIT
    ldr	    reg_tmpa, [reg_args, # ARGS_VA_IRQ_OFFSET]
    str	    reg_tmpd, [reg_tmpa, # S3C2410_SRCPND - S3C24XX_VA_IRQ ]

#if 0
    /* test code, change GPF0(BUZZER) */
    ldr	    reg_tmpa, [reg_args, # ARGS_VA_GPIO_OFFSET]
    ldr	    reg_tmpd, [reg_tmpa, # BUZ_GPXDAT_BASE - S3C24XX_VA_GPIO]
    eor	    reg_tmpd, reg_tmpd, # BUZ_PIN_MASK
    str	    reg_tmpd, [reg_tmpa, # BUZ_GPXDAT_BASE - S3C24XX_VA_GPIO]

    add	    reg_index, reg_index, # 1;
    add	    reg_datain, reg_datain, #1; 

    subs  pc, lr, # 4
#endif

    /* Check the running status */
    ldr	    reg_tmpd, [reg_args, # ARGS_STATUS_OFFSET]
    cmp	    reg_tmpd, # FIQ_STATUS_RUNNING
    subnes  pc, lr, # 4

    /* Test and save 1 bit pin value */
    ldr	    reg_tmpa, [reg_args, # ARGS_VA_GPIO_OFFSET]
    ldr	    reg_tmpd, [reg_tmpa, # DATA_GPXDAT_BASE - S3C24XX_VA_GPIO]
    lsr	    reg_tmpd, reg_tmpd, # DATA_PIN_NUM
    and	    reg_tmpd, reg_tmpd, # 1 
    orr	    reg_datain, reg_tmpd, reg_datain, lsl #1

    /* Bits flag +1 */
    add	    reg_index, reg_index, # 1

    /* Check if 1 pixel data has reveived */
//    lsl	    reg_tmpd, reg_index, # 16
//    lsr	    reg_tmpd, reg_tmpd, # 16
    and	    reg_tmpd, reg_index, # 0xff
    cmp	    reg_tmpd, # CCD_RES - 1
    bls	    out;

    /* After 1 pixel data received, clear bit flag, storage datain to buffer and increase cell index */
    lsr	    reg_index, reg_index, # 16
    str	    reg_datain, [reg_buffer, reg_index, lsl #2]
    mov	    reg_datain, # 0
    add	    reg_index, reg_index, # 1

    /* If we are over the edge of the buffer, clear the write index,
     * bits flag and change the running status
     */
    cmp   reg_index, # FIQ_BUFFER_LENGTH  
    movcs   reg_index, # 0
    movcs   reg_tmpd, # FIQ_STATUS_FINISHED
    strcs   reg_tmpd, [reg_args, # ARGS_STATUS_OFFSET]

    /* End of 1pixel recieve procedure */
    lsl   reg_index, reg_index, # 16

out:
    /*  return from FIQ */
    subs    pc, lr, # 4
fiq_end:
