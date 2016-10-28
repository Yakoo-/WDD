#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <errno.h>
#include <string.h>

#define ADC_SELMUX          0
#define WDD_ADC_CHANNEL     0
#define R1      4.7 // voltage dividing resistor kOm
#define R2      4.7 // Vin = Vbat * R2 * (R1 + R2)
#define RDVI    (R2 / (R1 + R2))
#define ADC_AVERAGE_ERROR       0.04
#define DEFAULT_REPEATE_TIME    500
#define N_TIMES_AVERAGE         4

#define ADC_DEV_NAME        "/dev/adc"

#include "oled.h"


void OLED_SetPos(int fd_oled, uchar x, uchar y)
{
    ioctl(fd_oled, OLED_CMD_SET_POS, x | (y << 8));
}

void OLED_WrDat(int fd_oled, uchar data)
{
    ioctl(fd_oled, OLED_CMD_WR_DAT, data);
}

void OLED_WrCmd(int fd_oled, uchar cmd)
{
    ioctl(fd_oled, OLED_CMD_WR_CMD, cmd);
}

void usage(void)
{
    fprintf(stderr, "Usage:adctest <auto/times>\n");
    exit(1);
}

#define BAT_IMAGE_WIDTH     (BATTERY_LEVEL_NUM + 4)
#define BATTERY_LEVEL_NUM   7   // 0 ~ 6
#define BAT_IMG_BLANK_INX   0
#define BAT_IMG_HEAD_INX    1 
#define BAT_IMG_FULL_INX    2 
#define BAT_IMG_VOL_INX     3

void print_vol_on_screen(int vol_lev)
{
    int fd_oled = -1, i = 0;
    char vol_image[] = { 0, 0x1c, 0x7f, 0x41 };
    char bits;
    int col = 0;

    fd_oled = open(OLED_DEVICE_PATH, O_RDWR);
    if(fd_oled < 0)
        Error("Can't open device: " OLED_DEVICE_PATH "\n");

    for (i = 0; i < BAT_IMAGE_WIDTH; i++){
        OLED_SetPos(fd_oled, OLED_WIDTH - BAT_IMAGE_WIDTH + i, 0);

        switch (i)
        {
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

        OLED_WrDat( fd_oled, bits );
    }

    close(fd_oled);
}

int vol_flag = 0;
const int vol_lv[] = {3400, 3550, 3600, 3650, 3720, 3800, 3900, 4300};  // voltage level, 340~430

int get_volflag(int voltage)
{
    int lv_num = 0;
    int i = 0;

    lv_num = sizeof(vol_lv)/sizeof(vol_lv[0]);

    if (voltage < vol_lv[0] || voltage > vol_lv[lv_num -1]){
        DEBUG_INFO("Invalid voltage: %d!\n", voltage);
        return -1;
    }

    for (i = 1;i < lv_num;i++){
        if (voltage < vol_lv[i])
            return i-1;
    }

    return -1;
}

int main(int argc, char *argv[])
{
    int fd = 0, i = 0, j = 0;;
    int err;
    int channel = WDD_ADC_CHANNEL;
    int repeate = 1;

    if ((argc != 1) && (argc != 2))
        usage();

    fd = open(ADC_DEV_NAME, O_RDONLY);
    if (fd < 0){
        DEBUG_INFO("Failed to open ADC device!\n");
        exit(1);
    }

    if (argc == 2){
        if ( !strcmp(argv[1], "auto") )
            repeate = DEFAULT_REPEATE_TIME;
        else
            repeate = atoi(argv[1]);

        DEBUG_INFO("Repeate time: %d\n", repeate);
    }


    err = ioctl(fd, ADC_SELMUX, (unsigned long)channel);
    if (err){
        DEBUG_INFO("Failed to set ADC channel!");
        close(fd);
        exit(1);
    }

    for(i = 0; i < repeate; i++){
        char buffer[30];
        float voltage = 0.0; 
        int adc_value = 0, volflag = 0;

        voltage = 0;
        for (j = 0; j < N_TIMES_AVERAGE; j++){
            int len = read(fd, buffer, sizeof(buffer) - 1);
            int tmp;
            if (len > 0){
                buffer[len] = '\0';
                sscanf(buffer, "%d", &tmp);
                adc_value += tmp;
            }else{
                DEBUG_INFO("Failed to read ADC value!\n");
                exit(1);
            }
            usleep(200000);
        }

        adc_value /= N_TIMES_AVERAGE;

        voltage = 3.3 * adc_value / 4095 / RDVI + ADC_AVERAGE_ERROR;
        volflag = get_volflag( (int)(voltage * 1000) );
        print_vol_on_screen( volflag );

        printf("%d\n", volflag);
        DEBUG_INFO("ADC Value: %d, Channel: %d, Voltage: %1.3fV, Vol-level: %d\n", adc_value, channel, voltage, volflag);

    }

adcstop:
    close(fd);
}
