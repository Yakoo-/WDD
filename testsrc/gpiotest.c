#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define DEBUG 1
#include "gpioctl.h"

static int fd;
static char *funcname;

void Error(char * errinfo)
{
    printf("%s: Deadly error occured!\nError info: %s\n", funcname, errinfo);
    exit(1);
}

const static int max_int = 0x7fffffff;
const static int min_int = 0xffffffff;
const static int max_int_div_10 = 0x7fffffff / 10;
const static int max_int_div_10_rem = 0x7fffffff % 10;
// make sure the input char array is end up with '\0'.
int str2int(const char * const str)
{
    int i = 0, sign = 1, num = 0, digit = 0;
    int len = strlen(str);

    /* ingore all whitespace */
    while (i < len && ' ' == str[i]) i++;

    if (i< len && '+' == str[i]){
        sign = 1;
        i++;
    }

    if (i < len && '-' == str[i]){
        sign = -1;
        i++;
    }

    while (i < len){
        digit = (str[i] >= '0' && str[i] <= '9') ? str[i] - '0' : -1;
        if (digit < 0)
            break;
        if (num > max_int_div_10 || (max_int_div_10 == num && digit > max_int_div_10_rem) )
            return sign ? max_int : min_int;
        num = num * 10 + digit;
        i++;
    }

    return sign * num;
}

unsigned int get_pin(char *str)
{
    int index = 0;
    int length = 0;
    char bank = 0;
    char *pin_name;
    unsigned int pin_num;
    unsigned int pin;

    // check arguments
    if (str == NULL)
	Error("NULL input in function get_pin");
    for (index=0; str[index]!='\0'; index++);
#ifdef DEBUG
    printf("Index: %d\n",index);
#endif
    if (index < 4)
	Error("Invalid input length in function get_pin");
    if ((str[0]!='g') && (str[0]!='G') && (str[1]!='p') && (str[1]!='P'))
	Error("Invalid input header in function get_pin");
    if ((str[2]<'A') || (str[2]>'M' && str[2]<'a') || (str[2]>'m'))
	Error("Invalid input gpio bank in function get_pin");

    pin_num = str2int(str+3);
    // uppercase or lowercase, reference to gpio-nrs.h
    pin = (str[2] - (str[2]<'a'?'A':'a')) * 32 + pin_num ;
    
#ifdef DEBUG
    printf("In get_pin: str: %s, pin_num: %d, pin: %d\n", str, pin_num, pin);
#endif
    
    return pin;
}

void usage()
{
    printf("Usage:\n");
    printf("  %s cfg <pin> <val>\n", funcname);
    printf("  %s set <pin> <val>\n", funcname);
    printf("  %s get <pin>\n", funcname);
    printf("eg: set pin GPE11 to output\n");
    printf("  %s cfg gpe11 1\n", funcname);
    printf("WARNING: Configuraton for each bank of gpio is not fixed! Check S3C2416 USER MANUAL--CHAPTER 10 carefully before you run this function!\n");
}

int main(int argc, char **argv)
{
    int cmd = 0;
    unsigned int pin = 0;
    unsigned int val = 0;
    int i = 0;

#ifdef DEBUG
    printf("argc: %d\n", argc);
    for (i=0; i<argc; i++){
	printf("argv[%d]: %s\n", i, argv[i]);
    }
#endif

    funcname = argv[0] + 2;
    // check arguments
    if ((argc == 3) && !strcmp(argv[1], "get")){
	cmd = IOCTL_GET_PIN;
	pin = get_pin(argv[2]);
	if (!pin){
	    Error("Failed to get pin value, check input arguments!");
	}
    }

    if ((argc == 4) ){
	if (strcmp(argv[1], "cfg"))
	    cmd = IOCTL_SET_CFG;
	if (strcmp(argv[1], "set"))
	    cmd = IOCTL_SET_PIN;
	pin = get_pin(argv[2]);
	val = str2int(argv[3]);
    }

    if (!cmd){
	usage(argv[0]);
	return -1;
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
	Error("Failed to open /dev/gpioctl");
    }

    switch (cmd)
    {
    case IOCTL_SET_CFG:
	ioctl(fd, (val << 4) | cmd, pin);
	break;
    case IOCTL_SET_PIN:
	ioctl(fd, (val << 4) | cmd, pin);
	break;
    case IOCTL_GET_PIN:
	val = ioctl(fd, cmd, pin);
#ifdef DEBUG
	printf("Pin: %s, Value: %d\n",argv[2],val);
#endif
	break;
    default:
	break;
    }

    close(fd);
    return 0;
}

