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

int main(void)
{
    int i;
    int button_fd;
    int key_value[4];

    button_fd = open("/dev/button",O_RDONLY);
    if(button_fd < 0)
    {
        perror("open device button\n");
        exit(1);
    }

    printf("Button test for COM2416, install module button.ko before run this program.\nPress Ctrl+C to exit.\n");
    for(;;) {
        fd_set rds;
        int ret;

        FD_ZERO(&rds);
        FD_SET(button_fd, &rds);

        ret = select(button_fd + 1, &rds, NULL, NULL, NULL);

        if (ret < 0) {
            perror("select in testbutton");
            exit(1);
        }

        if (ret == 0) {
            printf("Timeout.\n");
        } else if(FD_ISSET(button_fd, &rds)) {
            int ret=read(button_fd, key_value, sizeof(key_value));
            if (ret != sizeof(key_value)) {
                if (errno != EAGAIN)
                    perror("read device button\n");
               continue;
            } else {
            for (i = 0;i < 4;i++)
                if(key_value[i] != 0){
                    printf("K%d %s, key value = 0x%02x\n",i+1,(key_value[i] & 0x80) ? "released" : key_value[i] ? "pressed down" : "", key_value[i]);
                    if (key_value[i] & 0x80) 
                        system("./ccdtest");
                }
            }
        }
    }
    close(button_fd);
    return 0;
}




