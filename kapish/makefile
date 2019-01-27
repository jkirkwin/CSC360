CFLAGS=-Wall -Werror

default : kapish

test : test_kapish test_history 

test_kapish : kapish test_kapish.c
	gcc test_kapish.c -o test_kapish

test_history : test_history.c history.c history.h
	gcc test_history.c history.c -o test_history

relaxed : kapish.c history.c kapish.h history.h
	gcc kapish.c history.c -o kapish

debug : kapish.c history.c kapish.h history.h
	gcc $(CFLAGS) -DDEBUG=1 kapish.c history.c -o kapish

kapish : kapish.o history.o
	gcc $(CFLAGS) -o kapish kapish.o history.o

kapish.o : kapish.c kapish.h
	gcc $(CFLAGS) -c kapish.c

history.o : history.c history.h
	gcc $(CFLAGS) -c history.c

clean : 
	rm -rf *.exe *.o kapish test_history test_kapish