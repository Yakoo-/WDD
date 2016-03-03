#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#define BYTE unsigned char
usage(){
    printf( "\nUsage iotest devicepath address\n"
	    "Write 10 bytes of data to the specified address of specified device, read it and print out\n\n");
}   

int main(int argc, char **argv){
    int fd;
    char *devpath;
    long long address;
    long long size = 0;
    char data[] = "4982750370";
    BYTE *rdata;
    int ret;
    int i;
    int length;

    if (argc != 3) {
	usage();
	return 0;
    }
    
    //get arguments
    devpath = argv[1];
    address = atoll(argv[2]);
    length = 10 * sizeof(char);

    //open device for write
    fd = open(devpath, O_WRONLY);
    if (fd == -1){
	printf("Can not open %s\n",devpath);
	return -1;
    }else{
	if (ioctl(fd, BLKGETSIZE64, &size) == -1){
	    printf("Warning: %s\n", strerror(errno));
	    return -1;
	}
    }
    
    //judge the validity of input address
    if ((address + length) > size){
	printf("Address out of bound!");
	return -1;
    }

    //clear the data buffer

    ret = lseek(fd, address,SEEK_SET);
    ret = write(fd, data, length);
    close(fd);

    if (ret != length){
	perror("write error:\t");
	return -1;
    }

    rdata = (void *)malloc(length);
    memset((void *)rdata,0,length*sizeof(BYTE));

    fd = open(devpath, O_RDONLY);
    if (fd == -1){
	printf("Can not open device %s\n",devpath);
	return -1;
    }
    ret = lseek(fd, address,SEEK_SET);
    ret = read(fd, rdata, (size_t)length);

    if (ret != length){
	perror("write error:\t");
	return -1;
    }
    
    printf("Write 10 bytes of data to the specified address of a specified device, then read it and print out\n"
	   "data read out: \n");
    for (i = 0; i<length; i++){
	printf("%d ",(int)rdata[i]); 
    }
    printf("\n");

    free(rdata);
    
    return 0;
}
