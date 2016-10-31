#include <stdio.h>

#include "thread_battery.h"

#define BAT_IMAGE_WIDTH     (BATTERY_LEVEL_NUM + 4)
#define BATTERY_LEVEL_NUM   7   // 0 ~ 6
#define BAT_IMG_BLANK_INX   0
#define BAT_IMG_HEAD_INX    1 
#define BAT_IMG_FULL_INX    2 
#define BAT_IMG_VOL_INX     3
/*                        blank head  full  vol */
static const vol_image[] = { 0, 0x1c, 0x7f, 0x41 };

void fresh_battery(void)
{
    int i = 0, col = 0;
    char bits; 

    while(1){
        int vol_lev = process_adc();

        /* print battery image on screen */
        for (i = 0; i < BAT_IMAGE_WIDTH; i++){
            switch (i) {
            case 0:
                bits = vol_image[BAT_IMG_BLANK_INX];
                break;
            case 1:
                bits = vol_image[BAT_IMG_HEAD_INX];
                break;
            case 2:
            case (BAT_IMAGE_WIDTH - 1):
                bits = vol_image[BAT_IMG_FULL_INX];
                break;

            default:
                bits = (BATTERY_LEVEL_NUM - (i - 2)) > vol_lev ? vol_image[BAT_IMG_VOL_INX] : vol_image[BAT_IMG_FULL_INX];
                break;
            }

            OLED_WrBits(OLED_COL_NUM - BAT_IMAGE_WIDTH + i, 0, bits);
        }
        DEBUG_NUM(vol_lev);
        sleep(30);
    }
}
