/*
 * A stack of strings for history command support in Kapish
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_STACK_SIZE 25

int n = 0;
int stack_size = 0;
char **stack = NULL;
int initialized = 0;

void init_hist() {
    if(initialized) {
        return;
    }

    stack = (char**) malloc(sizeof(char*) * INITIAL_STACK_SIZE);
    if(!stack) {
        printf("Malloc Failed. Cannot initialize history stack.\n");
        exit(1);
    }
    stack_size = INITIAL_STACK_SIZE;
    n = 0;
    initialized = 1;
}

char *hist_pop() {
    if(!initialized || n < 1) {
        return NULL;
    }
    n--;
    return stack[n];
}

/*
 * Push a copy of the string passed
 */ 
void hist_push(char *cmd) {
    #ifdef DEBUG
        printf("Pushing %s into history stack\n", cmd);
    #endif
    if(!initialized) {
        init_hist();
    }
    if(stack_size <= n) {
        #ifdef DEBUG
            printf("Resizing stack\n");
        #endif
        stack = (char**) realloc(stack, sizeof(char*) * stack_size * 2);
        if(!stack) {
            printf("Realloc failed when increasing history stack size.\n");
            exit(1);
        }
        stack_size = stack_size * 2;
    }
    char * copy = (char*) malloc(sizeof(char) * (strlen(cmd)+1) );
    strncpy(copy, cmd, strlen(cmd));
    copy[strlen(cmd)] = '\0';
    #ifdef DEBUG
        printf("Copy of cmd: %s\n", copy);
    #endif
    stack[n] = copy;
    n++;
}

/* 
 * Indexed from the top of the stack, i.e. the end of the array
 * Position n-1 has index 0
 */
char *hist_get(int index) {
    if(!initialized || index > n-1 || index < 0) {
        return NULL;
    } else {
        char *match = stack[n-1-index];
        char *copy = (char *) (malloc(sizeof(char) * (strlen(match) + 1)));
        strncpy(copy, match, strlen(match));
        copy[strlen(match)] = '\0';
        return copy;
    }
}

/*
 * Return the index of the most recent command that begins with 
 * the specified prefix, -1 if no match exists.
 */
int match_prefix(char *prefix) {
    if(!initialized || !prefix) {
        return -1;
    }
    int index;
    int prefix_len = strlen(prefix);
    char *cmd;
    for(index = 0; index < n; index++) {
        cmd = stack[n-1-index];
        if(strlen(cmd) >= prefix_len && 0==strncmp(cmd, prefix, prefix_len)) {
            return index;
        }
    }
    return -1;
}

int hist_size() {
    return n;
}

/*
 * Empty and uninitialize the stack, free all memory used
 */ 
void clear_hist() {
    if(!initialized) {
        return;
    }
    int i;
    for(i = 0; i < n; i++) {
        free(stack[i]);
    }
    free(stack);
    n = 0;
    stack_size = 0;
    initialized = 0;
}

int is_initialized() {
    return initialized;
}