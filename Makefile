DIR_MOD  = ./modsrc
DIR_TEST = ./testsrc
DIR_OBJ = ./obj

all:
	((cd ${DIR_MOD} && make) || exit 1;)
	((cd ${DIR_TEST} && make) || exit 1;)
	cp ${DIR_MOD}/*.ko ${DIR_OBJ}
	cp ${DIR_TEST}/*.o ${DIR_OBJ}

modules:
	((cd ${DIR_MOD} && make) || exit 1;)
	cp ${DIR_MOD}/*.ko ${DIR_OBJ}

test:
	((cd ${DIR_TEST} && make) || exit 1;)
	cp ${DIR_TEST}/*.o ${DIR_OBJ}

clean:
	((cd ${DIR_MOD} && make clean) || exit 1;)
	((cd ${DIR_TEST} && make clean) || exit 1;)
	rm -f ${DIR_OBJ}/*

