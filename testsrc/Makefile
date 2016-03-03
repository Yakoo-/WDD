CC = arm-linux-gcc

SRC = $(wildcard *.c)
DIR = $(notdir ${SRC})
OBJ = $(patsubst %.c,%.o,${DIR}) 

.PHONY:all clean

all:$(OBJ)

.c.o:
	$(CC) -o $@ $<

clean:
	rm -f *.o
