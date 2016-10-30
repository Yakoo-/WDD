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
#include <pthread.h>

#include "common.h"
#include "main.h"
#include "button_ctrl.h"

static int fd_button = -1;

void select_button(void)
{
    int key_value[4], ret, i;
    fd_set rds;

    if( -1 == fd_button ){
        fd_button = open(BUTTON_DEVICE_PATH, O_RDONLY);
        if (fd_button < 0){
            perror("Failed to open button device\n");
            pthread_exit(0);
        }
    }

    while(1){
        FD_ZERO(&rds);
        FD_SET(fd_button, &rds);

        ret = select(fd_button + 1, &rds, NULL, NULL, NULL);

        if (ret < 0) {
            close(fd_button);
            perror("Failed to select button device");
            pthread_exit(0);
        }

        if (ret == 0) {
            printf("Timeout.\n");
        } 
        else 
            if(FD_ISSET(fd_button, &rds)) {
            int ret=read(fd_button, key_value, sizeof(key_value));
            if (ret != sizeof(key_value)) {
                if (errno != EAGAIN)
                    perror("read device button\n");
               continue;
            }

            pthread_mutex_lock( &repeate_lock );
            if (0 == repeate)
                pthread_cond_signal( &repeate_cond );

            for (i = 0;i < 4;i++){
                switch (key_value[i]) {
                case 0x81:
                    repeate = 1;
                    break;
                case 0x82:
                    repeate = 10;
                    break;
                case 0x83:
                    repeate = 20;
                    break;
                case 0x84:
                    break;
                default:
                    break;
                }
            }

            pthread_mutex_unlock( &repeate_lock );
        }
    }
}
