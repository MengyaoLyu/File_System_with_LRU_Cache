CFLAGS = -Wall
OBJECTS = \
	block_store.o \
	cachedisk.o \
	checkdisk.o \
	debugdisk.o \
	disk.o \
	ramdisk.o \
	statdisk.o \
	tracedisk.o \
	treedisk.o \
	treedisk_chk.o

all: trace chktrace

clean:
	rm -f *.o trace chktrace

trace: trace.o $(OBJECTS)
	$(CC) -o trace trace.o $(OBJECTS)

chktrace: chktrace.c
	$(CC) -o chktrace chktrace.c
