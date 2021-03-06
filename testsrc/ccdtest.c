#define DEBUG
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

#define OLED_CMD_INIT       0x01
#define OLED_CMD_CLEAR_ALL  0x02
#define OLED_CMD_CLEAR_ROW  0x03
#define OLED_CMD_FILL       0X04
#define OLED_CMD_SET_POS    0x05    //Y=row= (arg >> 8) & 0xff; X=col= arg & 0xff;
#define OLED_CMD_WR_DAT     0x06
#define OLED_CMD_WR_CMD     0x07
#define uchar8      unsigned char
#define uchar       unsigned char
#define ushort16    unsigned short
#define ushort      unsigned short

#ifdef DEBUG
#ifdef __KERNEL__
#define DEBUG_NUM(a)    printk(KERN_INFO "[%s:%d] "#a" =%d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...) printk(KERN_INFO "[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)

#else
#define DEBUG_NUM(a)    printf("[%s:%d] "#a" =%d\r\n",__func__,__LINE__,a)
#define DEBUG_INFO(fmt, args...) printf("[%s:%d]"#fmt"\n", __func__, __LINE__, ##args)
#endif

#else
#define DEBUG_NUM(a)
#define DEBUG_INFO(fmt, args...)
#endif


static unsigned int pixels[CCD_MAX_PIXEL_CNT];
static const unsigned int buffer_len = sizeof(pixels);
static int fd_oled;

void OLED_SetPos(uchar x, uchar y)
{
    ioctl(fd_oled, OLED_CMD_SET_POS, x | (y << 8));
}

void OLED_WrDat(uchar data)
{
    ioctl(fd_oled, OLED_CMD_WR_DAT, data);
}

void OLED_WrCmd(uchar cmd)
{
    ioctl(fd_oled, OLED_CMD_WR_CMD, cmd);
}

void Error(char * errinfo)
{
    printf("Deadly error occured, error info: %s\n", errinfo);
    exit(1);
}

static void print_page_on_screen()
{
    unsigned int col, row, i, aop = 0; // average of 8 pixels

    fd_oled = open("/dev/oled",O_RDWR);
    if(fd_oled < 0)
        Error("Can't open device: /dev/oled \n");

    ioctl(fd_oled, OLED_CMD_INIT, 0);
    col = 0;
    row = 0;
    for (col=0; col<127; col++){
        aop = 0;
        for (i=0; i<8; i++)
            aop += pixels[col * 8 + i];
        aop = aop >> 7;    // aop = aop/8/16, compress 10bit value to 6 bit(0~63)

        for (row=0; row<aop/8; row++){
            OLED_SetPos(col, 8 - row -1);
            OLED_WrDat(0xff);
        }
        OLED_SetPos(col, 8 - row -1);
        OLED_WrDat((unsigned char)(0xff00 >> (aop % 8)));
    }

    close(fd_oled);
}

int main (int argc, char ** argv)
{
    int fd_ccd, i, ret = 0;
    int max_repeate = 100;
    char isrepeate = 0;

    printf("Test program for S10077 LINEAR CCD Driver\n");

    fd_ccd = open("/dev/linear-ccd",O_RDONLY);
    if (fd_ccd < 0)
        Error("Failed to open device: /dev/linear-ccd!\n");

    if ((argc == 2) && !strcmp(argv[1], "auto"))
        isrepeate = 1;
    else
        isrepeate = 0;

    ret = read(fd_ccd, pixels, buffer_len);
    print_page_on_screen();

    for (i = 1; (isrepeate) && (i < max_repeate); i++){
        usleep(600000);
        printf("\nTrying to get image data for the %d th time.\n", i + 1);
        ret = read(fd_ccd, pixels, buffer_len);
        print_page_on_screen();
        DEBUG_NUM(ret);
    }

    close(fd_ccd);

    return 0;
}
