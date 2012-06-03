CFLAGS = -g -Wall -O2 -ffast-math -std=c99
CC = gcc
LIBS = -lm -lOpenCL
INCLUDES = -I../opencl11/
SRCS = opencl.c util.c liveness.c rand.c vertex.c

all: liveness

liveness: opencl.o util.o liveness.o rand.o vertex.o 
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SRCS} ${LIBS}

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $<

clean:
	rm -f *.o *~


.PHONY: all
# DO NOT DELETE
