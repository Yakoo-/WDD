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

#define DEBUG

#ifdef DEBUG
#define DEBUG_NUM(a) 	printf("[%s:%d] "#a" = %d\r\n",__func__,__LINE__,a) 
#define DEBUG_INFO(fmt, args...) printf("[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#else
#define DEBUG_NUM(a)
#define DEBUG_INFO(fmt, args...)
#endif

static unsigned int pixels[CCD_MAX_PIXEL_CNT];
static const unsigned int buffer_len = sizeof(pixels);

static void print_page(unsigned int *page)
{
    int col, row;

    for (row = 0; row<128; row++){
	printf("row%-3d:\t", row);
	for (col = 0; col<8; col++){
	    printf("%-4d\t", page[row*8 + col]);
	}
	printf("\n");
    }
}

void Error(char * errinfo)
{
    printf("Deadly error occured, error info: %s.\n", errinfo);
    exit(1);
}

int main (void)
{
    int ret = 0;
    int fd_ccd = 0;
    int timeout = 0;

    fd_ccd = open("/dev/linear-ccd",O_RDONLY); 
    if (fd_ccd < 0)
	Error("Failed to open /dev/linear-ccd!\n");

    printf("Test program for S10077 LINEAR CCD Driver\n");

    while (ret != buffer_len){
	printf("Trying to get image data for the %d th time.\n", timeout + 1);
	ret = read(fd_ccd, pixels, buffer_len);
	DEBUG_NUM(ret);
	timeout++;
    }

    printf("Data read out:\n");
#ifdef DEBUG
    print_page(pixels);
#endif

    close(fd_ccd);

    return 0;
}
