CFLAGS=-std=c11 -Wall

all: lib test

lib: vdisk.o
	
vdisk.o: vdisk.c vdisk.h
	gcc ${CFLAGS} -c vdisk.c

test: lib test_vdisk.c vdisk.o
	gcc ${CFLAGS} vdisk.o test_vdisk.c -o test_vdisk

clean: 
	rm -rf *.o *.exe