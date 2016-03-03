#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <string.h>

/*
int rtc_valid_tm(struct rtc_time *tm)
{
	if (tm->tm_year < 70
		|| ((unsigned)tm->tm_mon) >= 12
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
		|| ((unsigned)tm->tm_hour) >= 24
		|| ((unsigned)tm->tm_min) >= 60
		|| ((unsigned)tm->tm_sec) >= 60)
		return -EINVAL;

	return 0;
}
*/

int print_time(struct rtc_time *ptm)
{
    int err;

//    err = rtc_valid_tm(ptm);
    if (err){
	fprintf(stderr, " Invalid date/time struct\n");
	return err;
    }

    printf(
	"current time: "
	"%d-%02d-%02d %02d:%02d:%02d\n",
	ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
	ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    return 0;
}

int read_time(char *devpath, struct rtc_time *ptm)
{
    int fd;
    int err;

    if (devpath == NULL){
	fprintf(stderr, " Null device path"); 
	return -1;
    }

    fd = open(devpath, O_RDONLY);
    if (fd < 0){
	fprintf(stderr, " Failed to open device %s\n", devpath);
	return err;
    }
     
    err = ioctl(fd, RTC_RD_TIME, ptm);
    if (err){
	fprintf(stderr, " Failed to read rtc\n");
	close(fd);
	return err;
    }

//    err = rtc_valid_tm(ptm);
    if (err){
	fprintf(stderr, " Invalid date/time struct\n");
	close(fd);
	return err;
    }

    close(fd);
    return 0;
}

int set_time(char *devpath, struct rtc_time *ptm)
{
    int fd;
    int err;

    if (devpath == NULL){
	fprintf(stderr, " Null device path"); 
	return -1;
    }

//    err = rtc_valid_tm(ptm);
    if (err){
	fprintf(stderr, " Invalid date/time struct\n");
	return err;
    }

    fd = open(devpath, O_WRONLY);
    if (fd < 0){
	fprintf(stderr, " Failed to open device %s\n", devpath);
	return err;
    }
     
    err = ioctl(fd, RTC_SET_TIME, ptm);
    if (err){
	fprintf(stderr, " Failed to set rtc\n");
	close(fd);
	return err;
    }

    close(fd);
    return 0;
}

int main(void)
{
    int i;
    int err;
    char *devpath = "/dev/rtc0";
    struct rtc_time tm;

    printf("\n\trtc test for 2416\n"); 

    err = read_time(devpath, &tm);
    if (err){
	fprintf(stderr, " Faild to read time!");
	exit(1);
    }else{
	print_time(&tm);
    }
    
    tm.tm_year -=1;
    err = set_time(devpath, &tm);
    if (err){
	fprintf(stderr, " Faild to set time!");
	exit(1);
    }

    err = read_time(devpath, &tm);
    if (err){
	fprintf(stderr, " Faild to read time!");
	exit(1);
    }else{
	print_time(&tm);
    }

    tm.tm_year +=1;
    err = set_time(devpath, &tm);
    if (err){
	fprintf(stderr, " Faild to set time!");
	exit(1);
    }

    err = read_time(devpath, &tm);
    if (err){
	fprintf(stderr, " Faild to read time!");
	exit(1);
    }else{
	print_time(&tm);
    }

/*
    fd = open(devpath, O_RDWR);
    if (fd < 0){
	fprintf(stderr, " Failed to open device %s\n", devpath);
	return err;
    }
    
    err = ioctl(fd, RTC_RD_TIME, &tm);
    if (err){
	fprintf(stderr, " Failed to read rtc\n");
	close(fd);
	return err;
    }else{
	printf(
	    "current time\t: "
	    "%d-%02d-%02d %02d:%02d:%02d\n",
	    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	    tm.tm_hour, tm.tm_min, tm.tm_sec);
   }

    memcpy((void *)&ntm, (const void *)&tm, sizeof(tm));
    ntm.tm_year -= 1;
    err = ioctl(fd, RTC_SET_TIME, &ntm);
    if (err){
	fprintf(stderr, " Failed to set rtc\n");
	close(fd);
	return err;
    }else{
	printf(
	    "set time to\t: "
	    "%d-%02d-%02d %02d:%02d:%02d\n",
	    ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday,
	    ntm.tm_hour, ntm.tm_min, ntm.tm_sec);
    }

    err = ioctl(fd, RTC_SET_TIME, &tm);
    if (err){
	fprintf(stderr, " Failed to set rtc\n");
	close(fd);
	return err;
    }else{
	printf(
	    "restore time to\t: " 
	    "%d-%02d-%02d %02d:%02d:%02d\n",
	    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	    tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    err = ioctl(fd, RTC_RD_TIME, &tm);
    if (err){
	fprintf(stderr, " Failed to read rtc\n");
	close(fd);
	return err;
    }else{
	printf(
	    "current time\t: "
	    "%d-%02d-%02d %02d:%02d:%02d\n\n",
	    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	    tm.tm_hour, tm.tm_min, tm.tm_sec);
   }
*/

/* 
    printf(
	"current time :"
	"%d-%02d-%02d %02d:%02d:%02d UTC (%u)\n",
	tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	tm.tm_hour, tm.tm_min, tm.tm_sec,
	(unsigned int)tv.tv_sec);
*/

/*
    err = read(fd, &tm, sizeof(tm));
    if (err){
	fprintf(stderr, " Failed to read rtc fd\n");
	close(fd);
	return err;
    }
*/

/*
    err = rtc_valid_tm(&tm);
    if (err){
	fprintf(stderr, " Invalid date/time struct\n");
	close(fd);
	return err;
    }
*/


    return 0;
}
