PREFIX = ${HOME}/local
BINDIR = ${PREFIX}/bin
CC = cc
CFLAGS = -std=c99 -g -Wall -Wmissing-prototypes

BIN_EXECS = clsem
SCRIPTS = clsem.sh
EXECS = ${BIN_EXECS} ${SCRIPTS}

clsem : clsem.c
	${CC} ${CFLAGS} -o clsem clsem.c

install : ${EXECS}
	mkdir -p ${BINDIR}
	cp ${EXECS} ${BINDIR}

clean :
	rm -fr ${BIN_EXECS} *core* *.dSYM
