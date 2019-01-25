#include <stdlib.h>
#include <stdio.h>

#include "kapish.h"

#define BUFFSIZE 100
char error_buffer[BUFFSIZE];

int main() {
    printf("Running Kapish Tests\n");
    printf("====================\n\n");
    int i;
    for(i = 0; i < BUFFSIZE; i++) {
        error_buffer[i] = '\0';
    }
}