objs= threadlib.o logging.o tygetopt.o

AR=ar -rsc
CC=gcc
CFLAGS=-g -O2 -Wall -W -I../include  -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_ISOC99_SOURCE -D_GNU_SOURCE
#CFLAGS=-g -O -Wall -W -I../include -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

all:  libthreadlib.a test 

libthreadlib.a : $(objs)
	$(AR) libthreadlib.a $(objs)

clean:
	rm -f *.o ; rm -f *.a; rm -f test

test: test.o ${objs}
	${CC} -pthread -o $@ test.o ${objs} -lm

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

