/*
 * A short program to help me get a handle on catching 
 * a signal (ctrl+c)
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int);

int main() {
    signal(SIGINT, signal_handler);
    while(1) {
        int i;
        for(i = 0; i < 100000000; i++) {
            sleep(1);
            printf("-\n");
        }
    }
}

void signal_handler(int s) {
    signal(s, SIG_IGN); // Ignore signal for the duration of the handler
    if(s == SIGINT) {
        printf("CONTROL SEE AMIRITE\n");
    }
    signal(s, signal_handler); // Re-instate handler
}