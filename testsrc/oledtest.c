#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "Font_Libs.h"
#include "oled.h"

static int fd;

void OLED_SetPos(uchar x, uchar y)
{
    ioctl(fd, OLED_CMD_SET_POS, x | (y << 8));
}

void OLED_WrDat(uchar data)
{
    ioctl(fd, OLED_CMD_WR_DAT, data);
}

void OLED_WrCmd(uchar cmd)
{
    ioctl(fd, OLED_CMD_WR_CMD, cmd);
}

/*****************************************************************************
 函 数 名  : OLED_P6x8Char
 功能描述  : 显示一个6x8标准ASCII字符
 输入参数  : uchar8 ucIdxX col  显示的横坐标0~122
             uchar8 ucIdxY row 页范围0～7
             uchar8 ucData  显示的字符
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_P6x8Char(uchar8 ucIdxX, uchar8 ucIdxY, uchar8 ucData)
{
    uchar8 i, ucDataTmp;

    ucDataTmp = ucData-32;
    if(ucIdxX > 122)
    {
        ucIdxX = 0;
	if (ucIdxY < 7)
	    ucIdxY++;
	else
	    return;
    }

    OLED_SetPos(ucIdxX, ucIdxY);

    for(i = 0; i < 6; i++)
    {
        OLED_WrDat(Font_6x8[ucDataTmp][i]);
    }
}

/*****************************************************************************
 函 数 名  : OLED_P6x8Str
 功能描述  : 写入一组6x8标准ASCII字符串
 输入参数  : uchar8 ucIdxX       显示的横坐标0~122
             uchar8 ucIdxY       页范围0～7
             uchar8 ucDataStr[]  显示的字符串
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_P6x8Str(uchar8 ucIdxX, uchar8 ucIdxY, uchar8 ucDataStr[])
{
    uchar8 i, j, ucDataTmp;

    for (j = 0; ucDataStr[j] != '\0'; j++)
    {
        ucDataTmp = ucDataStr[j] - 32;
        if(ucIdxX > 122)
        {
            ucIdxX = 0;
            ucIdxY++;
        }

        OLED_SetPos(ucIdxX,ucIdxY);

        for(i = 0; i < 6; i++)
        {
            OLED_WrDat(Font_6x8[ucDataTmp][i]);
        }
        ucIdxX += 6;
    }

    return;
}

/*****************************************************************************
 函 数 名  : OLED_P8x16Str
 功能描述  : 写入一组8x16标准ASCII字符串
 输入参数  : uchar8 ucIdxX       为显示的横坐标0~120
             uchar8 ucIdxY       为页范围0～3
             uchar8 ucDataStr[]  要显示的字符串
 输出参数  : NONE
 返 回 值  : NONE
*****************************************************************************/
void OLED_P8x16Str(uchar8 ucIdxX, uchar8 ucIdxY, uchar8 ucDataStr[])
{
    unsigned int i, j, ucDataTmp;

    ucIdxY <<= 1;

    for (j = 0; ucDataStr[j] != '\0'; j++)
    {
        ucDataTmp = ucDataStr[j] - 32;
        if(ucIdxX > 120)
        {
            ucIdxX = 0;
            ucIdxY += 2;
        }
        OLED_SetPos(ucIdxX, ucIdxY);

        for(i = 0; i < 8; i++)
        {
            OLED_WrDat(Font_8x16[(ucDataTmp << 4) + i]);
        }

        OLED_SetPos(ucIdxX, ucIdxY + 1);

        for(i = 0; i < 8; i++)
        {
            OLED_WrDat(Font_8x16[(ucDataTmp << 4) + i + 8]);
        }
        ucIdxX += 8;

    }

    return;
}

void Error(char * errinfo)
{
    printf("Deadly error occured, error info: %s.\n", errinfo);
    exit(1);
}

// the input char array must be end up with '\0'.
int str2int(char *str)
{
    int result = 0;
    int index = 0;
    int unit = 0;

    if (str == NULL)
	Error("Invalid input in function str2int");
    for (index=0; str[index]!='\0'; index++){
	if ((str[index] < 48) || (str[index] > 57))
	    Error("Invalid input in function str2int");
	result = result * 10 + str[index] - 48;
    }

    return result;
}

void usage(char *name)
{
    printf("Usage:\n");
    printf("  %s init\n", name);
    printf("  %s clear\n", name);
    printf("  %s clear <row>\n", name);
    printf("  %s print <row> <col> <string>\n", name);
    printf("eg:\n");
    printf("  %s display 2 0 Yakoo\n", name);
    printf("PS: row is 0~3\n");
    printf("PS: col  is 0~120\n");
}

int main(int argc, char **argv)
{
    uint cmd = 0, oled_cmd = 0, oled_arg = 0;
    int row = -1;
    int col = -1;
    char *str;
    int light = 0;
    int i = 0;

    DEBUG_INFO("argc: %d\n", argc);
    for (i=0; i<argc; i++)
        DEBUG_INFO("argv[%d]: %s\n", i, argv[i]);
    
    // check arguments
    if ((argc == 2) && !strcmp(argv[1], "init"))
        cmd = 1;
    if ((argc == 3) && !strcmp(argv[1], "init")){
        cmd = 1;
        light = str2int(argv[2]);
        DEBUG_INFO("light: %d\n", light);
        light &= 0x01;
    }
    if ((argc == 2) && !strcmp(argv[1], "clear"))
        cmd = 20; 
    if ((argc == 3) && !strcmp(argv[1], "clear")){
        cmd = 21;
        row = str2int(argv[2]);
        DEBUG_INFO("row: %d\n", row);
        if (row < 0 || row > 3){
            printf("Invalid row, valid row is 0~3.\n");
            return -1;
        }
    }
    if ((argc == 5) && !strcmp(argv[1], "print")){
        cmd = 3;
        row = str2int(argv[2]);
        col = str2int(argv[3]);
        DEBUG_INFO("row: %d, col: %d\n", row, col);
        str = argv[4];
        if (row < 0 || row > 7){
            printf("Invalid row, valid row is 0~7.\n");
            return -1;
        }
        if (col< 0 || col > 120){
            printf("Invalid column, valid column is 0~120.\n");
            return -1;
        }
    }
    if (!cmd){
        usage(argv[0]);
        return -1;
    }

    fd = open(OLED_DEVICE_PATH, O_RDWR);
    if(fd < 0){
        printf("Can't open device: " OLED_DEVICE_PATH "\n");
        return -1;
    }

    switch (cmd){
    case 1 : // initial OLED
        oled_cmd = OLED_CMD_INIT;
        oled_arg = light;
        break;
    case 20:	// clear screen
        oled_cmd = OLED_CMD_CLR_ALL;
        oled_arg = 0;
        break;
    case 21:	// clear 1 row
        oled_cmd = OLED_CMD_CLEAR_ROW;
        oled_arg = row;
        break;
    case 3 :	// print a string 
        DEBUG_INFO("Prepare to print string, row: %d, col: %d, str: %s\n", row, col, str);
        OLED_P6x8Str(col, row, str);
	break;
    default:
        printf("Invalid command!");
        break;
    }

    if (oled_cmd && oled_arg)
        ioctl(fd, oled_cmd, oled_arg);

    close(fd);

    return 0;
}
