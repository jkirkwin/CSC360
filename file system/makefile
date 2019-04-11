CFLAGS=-std=c11 -Wall

all : vdisk.o file.o unittest test01

vdisk.o : ./disk/vdisk.c ./disk/vdisk.h
	gcc ${CFLAGS} -c ./disk/vdisk.c

file.o : ./io/file.c ./io/file.h
	gcc ${CFLAGS} -c ./io/file.c
	
unittest : vdisk.o file.o ./io/test_file.c ./disk/test_vdisk.c
	gcc ${CFLAGS} vdisk.o ./disk/test_vdisk.c -o test_vdisk
	gcc ${CFLAGS} file.o vdisk.o ./io/test_file.c -o test_file

run_unittest : unittest
	./test_vdisk
	./test_file

test01: ./apps/test01.c vdisk.o
	gcc ${CFLAGS} vdisk.o ./apps/test01.c -o test01

test02: ./apps/test02.c vdisk.o file.o
	gcc -c ./apps/test02.c
	gcc ${CFLAGS} vdisk.o file.o test02.o -o test02

clean : 
	rm -rf *.exe *.o ./*/*.o ./*/*.exe