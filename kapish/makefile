CFLAGS=-Wall -Werror

all : kapish

relaxed : kapish.c
	gcc kapish.c -o kapish

debug : kapish.c
	gcc $(CFLAGS) -DDEBUG=1 kapish.c -o kapish

kapish : kapish.o
	gcc $(CFLAGS) -o kapish kapish.o

kapish.o : kapish.c kapish.h
	gcc $(CFLAGS) -c kapish.c

clean : 
	rm -rf *.exe *.o kapish