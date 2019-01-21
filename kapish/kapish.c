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
#include <ctype.h> // isspace

#include "kapish.h"

char * builtin_cmds [] = { 
    "setenv\0",
    "unsetenv\0",
    "cd\0",
    "exit\0",
};

int main(int argc, char const *argv[]) {
    
    #ifdef DEBUG
        printf("Running in DEBUG mode\n");
        printf("=====================\n\n");
    #endif
   
    // Load config file (.kapish)

    // Loop until done
    main_loop();

    // Do teardown/termination

    return 0;
}

int main_loop() {
    #ifdef DEBUG
        printf("Main loop entered\n");
    #endif
    int status = 1;
    char *input_line;
    int line_len;
    char **tokens;
    int num_tokens;
    while(status) {
        printf("? ");
        input_line = get_input_line(); // one more than the number of actual characters
        line_len = strlen(input_line);
        tokens = tokenize(input_line, &num_tokens);
        
        // Tokens are not being stored correctly. The number of tokens is correct. 10:30am jan 21
        #ifdef DEBUG
            printf("Number of tokens recorded: %d\n", num_tokens);
            printf("Line length: %d\n", line_len);
            printf("First token: %s\n", *tokens);
            

            // printf("\n");
        #endif


        /* 
        int i;
        for(i = 0; i < num_tokens; i++) {
            printf("%s\n", *(tokens + i));
        }
        */
        // TODO check for a builtin, otherwise (try to) execute the specified binary file
        // TODO prevent control+c from terminating kapish
        // TODO Update Status
        free(input_line);

        //  TODO need to free tokens
    }
    #ifdef DEBUG
        printf("Main loop finished. Returning.\n");
    #endif
    return 0;
}

/*
 * Removes trailing whitespace from (null-terminated) string
 */
void chop(char *str) {
    #ifdef DEBUG
        printf("Chopping string: \'%s\'\n", str);
    #endif
    char *p = str + (strlen(str) - 1) * sizeof(char);
    while(isspace(*p)) {
        *p = '\0';
        p = p - sizeof(char);
    }
}

/* 
 * Get a line of input from stdin character-by-character to ensure we don't chop off the 
 * end of the input as we might when using a buffer.
 * Strips any trailing whitespace from the string before returning it.
 */
char* get_input_line() {
    #ifdef DEBUG
        printf("getting input line\n");
    #endif
    int buffsize = 1000;

    // have memory reserved for *input_line to *(input_line + buffsize - 1);

    char *input_line = (char *) emalloc(sizeof(char) * buffsize);
    int chars = 0;
    char c;
    do {
        c = getchar();
        if(chars > buffsize-3) {
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
    } while(c != EOF && c != '\n' && c != '\r');
    *(input_line+chars) = '\0';
    chop(input_line);
    #ifdef DEBUG
        printf("input line retrieved: %s\n", input_line);
    #endif
    return input_line;
}

/*
 * Tokenizes the string passed in, stores the number of tokens generated, and returns an array
 * of all tokens
 */
char** tokenize(char *str, int *num_tokens) {
    #ifdef DEBUG
        printf("tokenizing string\'%s\' of length %lu", str, strlen(str));
    #endif
    printf("in tokenize\n");

    int len = strlen(str);

    int buffer_increment = len/5;
    int buffer_size = buffer_increment;

    char **tokens = emalloc(buffer_size * sizeof(char*));
    char *token = strtok(str, " ");
    int tokens_stored = 0;

    while(token) {

        printf("token #%d = \'%s\'\n", tokens_stored, token);

        // store the token, resize buffer if necessary
        if(tokens_stored >= buffer_size) {
            buffer_size = buffer_size + buffer_increment;
            tokens = realloc(tokens, buffer_size*sizeof(char*));
            if(!tokens) {
                printf("Reallocation Failed\n");
                exit(3);
            }
        }
        *(tokens + (tokens_stored)*sizeof(char *)) = token;
        tokens_stored++;

        token = strtok(NULL, " ");
    }
    // Free up unnecessary memory
    tokens = realloc(tokens, tokens_stored*sizeof(char *));
    if(!tokens) {
        printf("Reallocation Failed\n");
        exit(3);
    }
    *num_tokens = tokens_stored;
    return tokens;
} 

/*
 * Error-handling wrapper for malloc
 */
void *emalloc(int size) {
    void *p = malloc(size);
    if(!p) {
        printf("Malloc Failed.");
        exit(1);
    }
    return p;
}

/* 
 * Prints the hex representation of each character until a null char is encountered.
 */ 
void print_hex(char *str) {
    int i;
    for(i = 0; i < strlen(str); i++) {
        printf("|%d| ", *(str+i));
    }
    printf("\n");
}

/*
 * Prints the output of isspace for each char before the \0
 */
void print_isspace(char *str) {
    int i;
    for(i = 0; i < strlen(str); i++) {
        printf("|%d| ", isspace(*(str + i)));
    }
    printf("\n");
}