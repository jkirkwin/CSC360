CFLAGS=-Wall -Werror

all : kapish

relaxed : kapish.c history.c
	gcc kapish.c history.c -o kapish

debug : kapish.c history.c
	gcc $(CFLAGS) -DDEBUG=1 kapish.c history.c -o kapish

kapish : kapish.o history.o
	gcc $(CFLAGS) -o kapish kapish.o history.o

kapish.o : kapish.c kapish.h
	gcc $(CFLAGS) -c kapish.c

history.o : history.c
	gcc $(CFLAGS) -c history.c

clean : 
	rm -rf *.exe *.o kapish