#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// Test getcwd result
int main() {
    char * buffer = (char *) malloc(100);
    buffer = getcwd(buffer, 100);
    printf(buffer);
}