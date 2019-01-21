CFLAGS=-Wall -Werror

all : kapish

kapish : kapish.o
	gcc $(CFLAGS) -o kapish kapish.o

kapish.o : kapish.c kapish.h
	gcc $(CFLAGS) -c kapish.c

clean : 
	rm -rf *.exe *.o