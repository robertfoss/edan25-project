CFLAGS += -g -Wall -Wextra -Werror -O2 -ffast-math -std=c99
CC = clang
LIBS = -lm -lOpenCL -pthread
INCLUDES = -I../opencl11/
SRCS = opencl.c util.c rand.c dataflow.c list.c

.PHONY: all clean

all: liveness

liveness: opencl.o util.o rand.o dataflow.o list.o
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SRCS} ${LIBS}

clean:
	rm -f *.o *~

