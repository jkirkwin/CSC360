all: lib test

lib: vdisk.o
	

vdisk.o: vdisk.c vdisk.h
	gcc -c vdisk.c

test: lib test_vdisk.c
	gcc vdisk.c -c
	gcc vdisk.o test_vdisk.c -o test_vdisk

clean: 
	rm -rf *.o *.exe