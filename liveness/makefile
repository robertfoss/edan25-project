CFLAGS += -std=c99 -g -Wall -Wextra -Werror -Wno-unused-parameter -O2 -ffast-math 
CC = gcc
LIBS = -lm -lOpenCL -pthread
INCLUDES = -I../opencl11/
SRCS = opencl.c util.c rand.c dataflow.c list.c

.PHONY: all clean

liveness: opencl.o util.o rand.o dataflow.o list.o
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SRCS} ${LIBS}
	rm -f *.o

liveness-opencl: opencl.o util.o rand.o dataflow.o list.o
	${CC} ${CFLAGS} -DUSE_OPENCL ${INCLUDES} -o $@ ${SRCS} ${LIBS}
	rm -f *.o

clean:
	rm -f *.o *~

