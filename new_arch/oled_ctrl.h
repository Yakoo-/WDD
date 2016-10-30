#pragma once

extern const unsigned char Font_6x8[][6];
extern const unsigned char Font_8x16[];

void OLED_WrBits(uchar col, uchar row, uchar bits);
void OLED_P6x8Char (uchar8 col, uchar8 row, uchar8 ucData);
void OLED_P6x8Str  (uchar8 col, uchar8 row, uchar8 str[] );
void OLED_P8x16Char(uchar8 col, uchar8 row, uchar8 ucData);
void OLED_P8x16Str (uchar8 col, uchar8 row, uchar8 str[] );
void OLED_PrintPattern(uchar8 col, uchar8 row, uchar8 *pattern, uint length);
void OLED_Init(void);
