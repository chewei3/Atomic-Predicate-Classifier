CC = gcc
CFLAGS = -g -lbdd
INC = -I include
INC_PATH = include

a.out: main.o  readroute.o readrule.o bddwrapper.o
	${CC} main.o readroute.o readrule.o bddwrapper.o ${CFLAGS} ${INC} -o APV
main.o: main.c ${INC_PATH}/readroute.h
	${CC} main.c ${CFLAGS} ${INC} -c
readroute.o: readroute.c ${INC_PATH}/readroute.h 
	${CC} readroute.c ${CFLAGS} ${INC} -c
readrule.o: readrule.c ${INC_PATH}/readroute.h  ${INC_PATH}/readrule.h
	${CC} readrule.c ${CFLAGS} ${INC} -c
bddwrapper.o : bddwrapper.c ${INC_PATH}/bddwrapper.h 
	${CC} bddwrapper.c ${CFLAGS} ${INC} -c
clean:
	@rm -rf *.o a.out
