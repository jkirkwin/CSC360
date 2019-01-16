source = linkedlist.h linkedlist.c
objects = linkedlist.o
executables = linkedlist.exe

all : linkedlist.c linkedlist.h
	gcc linkedlist.c -o linkedlist

debug : linkedlist.c linkedlist.h
	gcc -DDEBUG=1 linkedlist.c -o linkedlist	

clean :
	rm -rf $(objects) $(executables)