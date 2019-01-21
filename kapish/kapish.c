/*
 * Jamie Kirkwin
 * jkirkwin 
 * V00875987
 * CSC360 Assignment 1, Kapish Shell
 * Modified Jan 20, 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kapish.h"

// An array of pointers...
char * builtin_cmds [] = { 
    "setenv\0",
    "unsetenv\0",
    "cd\0",
    "exit\0",
};

int main(int argc, char const *argv[])
{
    printf("Kapish running\n");
    // Load config file (.kapish)


    // Loop until done
    main_loop();

    // Do teardown/termination

    return 0;
}

int main_loop() {
    int status = 1;//, i = 0;
    char *input_line;
    int line_len;
    // char **tokens;
    while(status) {
        printf("? ");
        input_line = get_input_line();
        line_len = strlen(input_line);
        printf("input line:  %s\n", input_line);
        printf("line length: %d\n", line_len);

        // TODO tokenize the line
        // strtok(input_line, 's')

        // TODO check for a builtin, otherwise (try to) execute the specified binary file
        // TODO prevent control+c from terminating kapish
    }


    return 0;
}

// get the input line character-by-character to ensure we don't chop off the end of the input
char * get_input_line() {
    int buffsize = 1000;
    char *input_line = (char *) emalloc(sizeof(char) * buffsize);
    int chars = 0;
    char c;
    do {
        c = getchar();
        if(chars > buffsize-2) {
        // increase input_line memory
        buffsize = buffsize * 2;
        input_line = (char *) realloc(input_line, buffsize);
        if(!input_line) {
            printf("Realloc Failed\n");
            exit(2);
        }
    }
    *(input_line + chars) = c;
    chars++;
    }while(c != EOF && c != '\n');
    *(input_line+chars) = '\0';
    return input_line;
}

void *emalloc(int size) {
    void *p = malloc(size);
    if(!p) {
        printf("Malloc Failed.");
        exit(1);
    }
    return p;
}