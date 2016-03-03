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

#define ADC_SELMUX  0
#define R1	    4.5	// voltage dividing resistor kOm
#define R2	    4.5	// Vin = Vbat * R2 * (R1 + R2)
#define RDVI	    R2 * (R1 + R2)

void usage(void)
{
    fprintf(stderr, "Usage:adctest [channel] \n");
    exit(1);
}

int vol_flag = 0;
const int vol_lv[] = {340, 365, 372, 380, 390, 430};  // voltage level, 340~430

int get_volflg(int voltage)
{
    int lv_num = 0;
    int i = 0;

    lv_num = sizeof(vol_lv)/sizeof(vol_lv[0]);
    if (voltage < vol_lv[0] || voltage > vol_lv[lv_num -1]){
	printf("Invalid voltage: %d!\n", voltage);
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
    int fd;
    int err;
    int channel = 0;
    float voltage = 0.0; 
    int volflag = 0;

    if ((argc != 1) && (argc != 2))
	usage();

    fd = open("/dev/adc", O_RDONLY);
    if (fd < 0){
	printf("Failed to open ADC device!\n");
	exit(1);
    }

    if (argc == 2){
	channel = atoi(argv[1]);
	if (channel > 9){
	   printf("Channel number must be 0~9!\n");
	   exit(1);
	}else
	    printf("Input channel: %d\n",channel);
    }else{
	printf("Use default channel: %d\n",channel);
    }


    err = ioctl(fd, ADC_SELMUX, (unsigned long)channel);
    if (err){
	printf("Failed to set ADC channel!");
	close(fd);
	exit(1);
    }

    for(;;){
	char buffer[30];
	int len;

	len = read(fd, buffer, sizeof(buffer) - 1);
	if (len > 0){
	    buffer[len] = '\0';
	    int value;
	    sscanf(buffer, "%d", &value);
	    voltage = 3.3 * value / 4095 / RDVI;
	    volflag = get_volflg((int)(voltage * 100));
	    printf("ADC Value: %d, Channel: %d, Voltage: %4.2fV, Vol-level: %d\n", value, channel, voltage, volflag);
	}else{
	    printf("Failed to read ADC value!\n");
	    exit(1);
	}
	sleep(1);
    }

adcstop:
    close(fd);
}
