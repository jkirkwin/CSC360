CFLAGS=-std=c11 -Wall

all: lib test

lib: file.o
	
file.o: file.c file.h
	gcc ${CFLAGS} -c file.c

test: lib test_file.c
	gcc ${CFLAGS} file.o test_file.c -o test_file

clean: 
	rm -rf *.o *.exe