#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#define CCD_MAX_PIXEL_CNT   1024
static unsigned int pixels[CCD_MAX_PIXEL_CNT];
static const unsigned int buffer_len = sizeof(pixels);

void Error(char * errinfo)
{
    printf("Deadly error occured, error info: %s.\n", errinfo);
    exit(1);
}

void print_array(unsigned int *array, unsigned int len)
{
    int index = 0;
    printf("\t\t 0\t 1\t 2\t 3\t 4\t 5\t 6\t 7\t 8\t 9");
    for (index=0; index<len; index++){
	if (index % 10 == 0)
	    printf("\n[%04d]",index / 10);
	printf("%03X",array[index]);
    }
    printf("\n\n");
}

int main (void)
{
    int ret = 0;
    int fd_ccd = 0;

    fd_ccd = open("/dev/linear-ccd",O_RDONLY); 
    if (fd_ccd < 0)
	Error("Failed to open /dev/linear-ccd!\n");

    printf("Test program for S10077 LINEAR CCD Driver\n");

    ret = read(fd_ccd, pixels, buffer_len);
    if (ret != buffer_len)
	Error("Data read out is too short\n");

    printf("Data read out:\n");
    print_array(pixels,CCD_MAX_PIXEL_CNT);

    close(fd_ccd);

    return 0;
}
