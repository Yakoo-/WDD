DIR_MOD  = ./modsrc
DIR_TEST = ./testsrc
DIR_NEW  = ./new_arch
DIR_OBJ  = ./obj
DIR_ALGO = ./algorithmsrc
OBJ = mod new test

.PHONY: all clean

all: $(OBJ)
	((cd ${DIR_ALGO} && make) || exit 1;)

mod:
	((cd ${DIR_MOD} && make) || exit 1;)
	cp -u ${DIR_MOD}/*.ko ${DIR_OBJ}

new:
	((cd ${DIR_NEW} && make) || exit 1;)
	cp -u ${DIR_NEW}/*.o ${DIR_OBJ}

test:
	((cd ${DIR_TEST} && make) || exit 1;)
	cp -u ${DIR_TEST}/*.o ${DIR_OBJ}

clean:
	((cd ${DIR_MOD} && make clean) || exit 1;)
	((cd ${DIR_TEST} && make clean) || exit 1;)
	((cd ${DIR_NEW} && make clean) || exit 1;)
	rm -f ${DIR_OBJ}/*.o ${DIR_OBJ}/*.ko

