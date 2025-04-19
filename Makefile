PROG=		img2p6screen3
SRCS=		img2p6screen3.c
OBJS=		${SRCS:.c=.o}

CFLAGS=		-O
LDFLAGS=

${PROG}:	${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS}

clean:
	rm -f ${PROG} *.o *.core
