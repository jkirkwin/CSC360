CFLAGS=-std=c11 -Wall

vdisk.o : ./disk/vdisk.c ./disk/vdisk.h
	gcc ${CFLAGS} -c ./disk/vdisk.c

file.o : ./io/file.c ./io/file.h
	gcc ${CFLAGS} -c ./io/file.c
	
unittest : vdisk.o file.o
	gcc ${CFLAGS} ./disk/vdisk.o ./disk/test_vdisk.c -o test_vdisk
	gcc ${CFLAGS} ./io/file.o ./io/test_file.c -o test_file

clean : 
	rm -rf *.exe *.o ./apps/*/.o ./apps/*/.exe ./io/*/.o ./io/*/.exe ./disk/*/.o ./disk/*/.exe