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

#include "ccd_ctrl.h"
#include "adc_ctrl.h"
#include "oled.h"

static int fd_adc = -1;

int process_adc(void)
{
    if (adc_init(WDD_ADC_CHANNEL) < 0)
        return -1;

    int adc_value = get_adc_value();
    float voltage = get_voltage( adc_value );

    return get_vol_level(voltage);
}

int get_vol_level(float vol)
{
    const int vol_lv[] = {3400, 3550, 3600, 3650, 3720, 3800, 3900, 4300};  // voltage level, 340~430
    int lv_num = 0;
    int i = 0;
    int voltage = (int)(vol * 1000);

    lv_num = sizeof(vol_lv)/sizeof(vol_lv[0]);

    if (voltage < vol_lv[0] || voltage > vol_lv[lv_num -1]){
        DEBUG_INFO("Invalid voltage: %d!\n", voltage);
        return -1;
    }

    for (i = 1; i < lv_num; i++){
        if (voltage < vol_lv[i])
            return i - 1;
    }

    return -1;
}

inline float get_voltage(int adc_value)
{
    return 3.3 * adc_value / 4095 / RDVI + ADC_AVERAGE_ERROR;
}

int get_adc_value(void)
{
    int i = 0, adc_value = 0, tmp = 0;

    for (i = 0; i < N_TIMES_AVERAGE; i++){
        read(fd_adc, &tmp, sizeof(tmp));

        if (tmp > 0)
            adc_value += tmp;
        else
            DEBUG_INFO("Failed to read ADC value!");

        usleep(100000);
    }

    return adc_value / N_TIMES_AVERAGE;
}

int adc_init(int channel)
{
    int err;
    if (fd_adc < 0)
        fd_adc = open(ADC_DEVICE_PATH, O_RDONLY);

    if (fd_adc < 0){
        DEBUG_INFO("Failed to open ADC device!\n");
        return -1;
    }

    err = ioctl(fd_adc, ADC_CMD_SELMUX, channel);
    if (err){
        DEBUG_INFO("Failed to set ADC channel!");
        close(fd_adc);
        return -1;
    }

    return 0;
}
