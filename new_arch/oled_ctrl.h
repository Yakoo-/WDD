#pragma once
#include <pthread.h>

#include "common.h"
#include "oled.h"

extern const unsigned char Font_6x8[][6];
extern const unsigned char Font_8x16[];
extern pthread_mutex_t oled_lock;

/* no protection here, use acquire/release_oled to protect critical region */
/* DO NOT ATTEMPT TO ACQUIRE OLED LOCK WHEN YOU HAVE ACQUIRED IT ALREADY */
void OLED_Init(void);
void OLED_SetPos(uchar col, uchar row);
void OLED_WrDat(uchar data);
void OLED_WrCmd(uchar cmd);

/* acquire/release mutex lock */
inline void acquire_oled(void);
inline void release_oled(void);

/* packet OLED operate functions */
void OLED_WrBits(uchar col, uchar row, uchar bits);
void OLED_P6x8Char (uchar8 col, uchar8 row, uchar8 ucData);
void OLED_P6x8Str  (uchar8 col, uchar8 row, uchar8 str[] );
void OLED_P8x16Char(uchar8 col, uchar8 row, uchar8 ucData);
void OLED_P8x16Str (uchar8 col, uchar8 row, uchar8 str[] );
void OLED_PrintPattern(uchar8 col, uchar8 row, uchar8 *pattern, uint length);
