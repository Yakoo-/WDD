KERN_DIR = /home/yakoo/linux_dev/linux-2.6.39.3

all:
	make -C $(KERN_DIR) M=`pwd` modules
		
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf module.order
			    
obj-m	+= hello.o
obj-m	+= led.o
obj-m	+= button.o
obj-m	+= adc.o
obj-m	+= spi_oled_drv.o
obj-m	+= spi_s3c2416_gpio.o
obj-m	+= timermod.o
obj-m	+= gpioctl.o
#obj-m	+= spi_s3c2416.o

obj-m	+= linearccd_fiq.o
linearccd_fiq-objs	:= fiq_handler.o linearccd.o

#obj-m	+= fiq_test.o
#fiq_test-objs	:= fiq_handler.o fiqtest.o

#obj-m	+= spi_s3c2440.o
