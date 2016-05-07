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

static void print_page(unsigned int *page)
{
    int col, row;

    for (row = 0; row<128; row++){
	printf("row%-3d:\t", row);
	for (col = 0; col<8; col++){
	    printf("0x%-4X\t", page[row*8 + col]);
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

    fd_ccd = open("/dev/linear-ccd",O_RDONLY); 
    if (fd_ccd < 0)
	Error("Failed to open /dev/linear-ccd!\n");

    printf("Test program for S10077 LINEAR CCD Driver\n");

    ret = read(fd_ccd, pixels, buffer_len);
    if (ret != buffer_len)
	Error("Data read out is too short\n");

    printf("Data read out:\n");
    print_page(pixels);

    close(fd_ccd);

    return 0;
}
