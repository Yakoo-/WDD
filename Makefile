DIR_MOD  = ./modsrc
DIR_TEST = ./testsrc
DIR_OBJ = ./obj
DIR_UPLOAD = ./upload
DIR_ALGO = ./algorithmsrc

all:
	((cd ${DIR_MOD} && make) || exit 1;)
	((cd ${DIR_TEST} && make) || exit 1;)
	((cd ${DIR_ALGO} && make) || exit 1;)
	cp -u ${DIR_MOD}/*.ko ${DIR_OBJ}
	cp -u ${DIR_TEST}/*.o ${DIR_OBJ}
	cp -u ${DIR_OBJ}/* ${DIR_UPLOAD}

modules:
	((cd ${DIR_MOD} && make) || exit 1;)
	cp -u ${DIR_MOD}/*.ko ${DIR_OBJ}

test:
	((cd ${DIR_TEST} && make) || exit 1;)
	cp -u ${DIR_TEST}/*.o ${DIR_OBJ}

clean:
	((cd ${DIR_MOD} && make clean) || exit 1;)
	((cd ${DIR_TEST} && make clean) || exit 1;)
	rm -f ${DIR_OBJ}/*

