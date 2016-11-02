#pragma once

#define BUTTON_DEVICE_NAME  "button"
#define BUTTON_DEVICE_PATH  "/dev/button"

extern pthread_mutex_t button_lock;
extern pthread_cond_t  button_cond;
extern unsigned int key_num;

void select_button(void *release_wake);
