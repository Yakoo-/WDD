CC = arm-linux-gcc

OBJ = ccdctrl

.PHONY:all clean

all:$(OBJ)

ccdctrl:
	$(CC) ccd_ctrl.c adc_ctrl.c oled_ctrl.c thread_button.c thread_battery.c -o ccdctrl.o -lfftw3f -lpthread

clean:
	rm -f *.o
