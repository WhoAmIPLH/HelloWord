CROSS_COMPILE ?=
CXX = $(CROSS_COMPILE)gcc
PWD = .
SRC_DIR = ${PWD}
OBJ_DIR = ${PWD}
INC_DIR = ${PWD}
BIN_DIR = ${PWD}

OBJS = \
	${OBJ_DIR}/main.o \
	${OBJ_DIR}/config.o \
	${OBJ_DIR}/log.o

TARGET = ${BIN_DIR}/a.out
INC_OPT = -I${INC_DIR}
LNK_OPT = -lpthread
TEST_OPT = -g

${TARGET} : chkobjdir chkbindir ${OBJS}
	${CXX} ${TEST_OPT} -o $@ ${OBJS} ${LNK_OPT}
	rm -f ${OBJ_DIR}/*.o

${OBJ_DIR}/%.o : ${SRC_DIR}/%.c
	${CXX} ${INC_OPT} ${TEST_OPT} -c -o $@ $<

chkobjdir :
	@if test ! -d ${OBJ_DIR} ; \
	then \
		mkdir ${OBJ_DIR} ; \
	fi

chkbindir :
	@if test ! -d ${BIN_DIR} ; \
	then \
		mkdir ${BIN_DIR} ; \
	fi


.PHONY : clean
clean :
	- rm -f ${TARGET}
	- rm -f ${OBJ_DIR}/*.o
