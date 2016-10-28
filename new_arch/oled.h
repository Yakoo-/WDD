#pragma once

#include <linux/ioctl.h>

#include "common.h"

#define OLED_DEVICE_NAME    "oled"
#define OLED_DEVICE_PATH    "/dev/oled"

/* oled resulution */
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_BITS_PER_CMD   8
#define OLED_ROW_NUM        (OLED_HEIGHT / OLED_BITS_PER_CMD)
#define OLED_COL_NUM        OLED_WIDTH
#define OLED_BYTES_NUM      (OLED_COL_NUM* OLED_ROW_NUM)

/* oled commands */
#define OLED_IOC_MAGIC      'z'
#define OLED_CMD_INIT       _IOW(OLED_IOC_MAGIC, 0, int)
#define OLED_CMD_CLR_ALL    _IOW(OLED_IOC_MAGIC, 1, int)
#define OLED_CMD_CLEAR_ROW  _IOW(OLED_IOC_MAGIC, 2, int)
#define OLED_CMD_FILL       _IOW(OLED_IOC_MAGIC, 3, int)
#define OLED_CMD_SET_POS    _IOW(OLED_IOC_MAGIC, 4, int)     //Y=row= (arg >> 8) & 0xff; X=col= arg & 0xff;
#define OLED_CMD_WR_DAT     _IOW(OLED_IOC_MAGIC, 5, int)
#define OLED_CMD_WR_CMD     _IOW(OLED_IOC_MAGIC, 6, int)
#define OLED_CMD_CLR_EXCEPT _IOW(OLED_IOC_MAGIC, 7, int)
#define OLED_IOC_MAXNR      9

