#pragma once

#include "common.h"
#include "oled.h"

#define DEFAULT_REPEATE_TIME    500

#define CCD_DEV_NAME        "/dev/linear-ccd"
#define CCD_MAX_PIXEL_CNT   1024
/* masking useless pixel data from left and right edge */
/* must be less than (CCD_MAX_PIXEL_CNT - OLED_WIDTH) / 2 = 448 */ 
#define CCD_EDGE_MASK       150
/* mask out bottom row to show title */
#define CCD_BOTTOM_MASK     1
#define CCD_RESOLUTION      10
#define CCD_MAX_PIXEL_VAL   (1 << CCD_RESOLUTION)
#define CCD_COMPRESS_RATE   (CCD_MAX_PIXEL_VAL / OLED_HEIGHT )
