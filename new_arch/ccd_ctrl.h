#pragma once

#include "common.h"
#include "oled.h"

#define DEFAULT_REPEATE_TIME    500

#define CCD_DEV_NAME        "/dev/linear-ccd"
#define CCD_MAX_PIXEL_CNT   1024
/* masking useless pixel data from left and right edge */
/* must be less than (CCD_MAX_PIXEL_CNT - OLED_WIDTH) / 2 = 448 */ 
#define CCD_EDGE_MASK       150
#define CCD_RESOLUTION      10
#define CCD_MAX_PIXEL_VAL   (1 << CCD_RESOLUTION)
#define CCD_COMPRESS_RATE   (CCD_MAX_PIXEL_VAL / OLED_HEIGHT )


/* mask out one row to show title */
/* for white background, like white paper,  it's better to use CCD_BOTTOM_MASK */
/* for black background, like denim fabric, it's better to use OLED_TOP_MASK */
#define OLED_TOP_MASK       1   // mask out the first row of OLED
#define CCD_BOTTOM_MASK     0   // mask out the last row data of CCD (aop)
/* #define OLED_TOP_MASK       0   // mask out the first row of OLED */
/* #define CCD_BOTTOM_MASK     1   // mask out the last row data of CCD (aop) */
