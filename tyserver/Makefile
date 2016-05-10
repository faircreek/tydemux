TIVO_CC=/usr/local/tivo/bin/gcc
TIVO_AR=/usr/local/tivo/bin/ar
TIVO_CFLAGS = -Wall -W -I../include -DTIVO -O2 -msoft-float -DTIVOSERVER
TIVO_CC_C= $(TIVO_CC) -c $(TIVO_CFLAGS)

COMMON= tyserver.c readfile.c image.c tyindex.c

all: tyserver tyindex

tyserver: tyserver.o ../libs/libmfs/libmfs.a readfile.o ../tydemux/libttydemux.a ../libs/libtlogging.a image.o
	$(TIVO_CC) -o tyserver $(LDFLAGS) tyserver.o \
				readfile.o -L ../libs/libmfs/ -lmfs -L../tydemux/ -lttydemux \
				-L../libs -ltlogging image.o

tyindex: tyindex.o ../libs/libmfs/libmfs.a readfile.o ../tydemux/libttydemux.a ../libs/libtlogging.a
	$(TIVO_CC) -o tyindex $(LDFLAGS) tyindex.o \
				readfile.o -L ../libs/libmfs/ -lmfs -L../tydemux/ -lttydemux \
				-L../libs -ltlogging 


readfile.o: readfile.c 
	$(TIVO_CC_C) -o readfile.o readfile.c

tyserver.o: tyserver.c image.c readfile.c 
	$(TIVO_CC_C) -o tyserver.o tyserver.c

image.o:
	$(TIVO_CC_C) -o image.o image.c

tyindex.o: image.c readfile.c 
	$(TIVO_CC_C) -o tyindex.o tyindex.c

clean:
	rm -f *~ *.o *.core *.BAK tyserver tyindex
