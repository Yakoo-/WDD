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
#include "thread_button.h"

static int fd_button = -1;
pthread_mutex_t button_lock;
pthread_cond_t  button_cond;
unsigned int key_num;

/* input : release_wake  trigger condition; = 0, press trigger; = 1, release trigger */
void select_button(void *release_wakep)
{
    int key_value[4], ret, i;
    int release_wake = *(int *)release_wakep;
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
        else if(FD_ISSET(fd_button, &rds)) {
            int ret=read(fd_button, key_value, sizeof(key_value));

            if (ret != sizeof(key_value)) {
                if (errno != EAGAIN)
                    perror("read device button\n");
               continue;
            }

            pthread_mutex_lock( &button_lock );

            for (i = 0; i < 4; i++){
                if (  key_value[i] &&   
                    ( release_wake &&  (key_value[i] & 0xf0)) ||
                    (!release_wake && !(key_value[i] & 0xf0))
                    ){
                        key_num = i + 1;
                        pthread_cond_signal( &button_cond );
                        break;
                    }
            }

            pthread_mutex_unlock( &button_lock );
        }
    }
}
