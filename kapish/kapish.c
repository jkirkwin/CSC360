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
#include <unistd.h> // chdir

#include "kapish.h"

typedef struct mapping {
    char *name;
    int (*handler)(int, char**);
} mapping_t;

#define BUILTINS 4
mapping_t mappings[] = {
    {"cd\0", &builtin_cd},
    {"exit\0", &builtin_exit},
    {"setenv\0", &builtin_setenv},
    {"unsetenv\0", &builtin_unsetenv}
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
    #define MAX_PATH_LENGTH 100
    char pathbuff[MAX_PATH_LENGTH + 1]; 
    int status = 0;
    char *input_line;
    char **tokens;
    int num_tokens;
    while(0 == status) {

        printf("%s ? ", getcwd(pathbuff, MAX_PATH_LENGTH));
        input_line = get_input_line();
        tokens = tokenize(input_line, &num_tokens);
        #ifdef DEBUG
            printf("Number of tokens recorded: %d\n", num_tokens);
            int i;
            printf("Tokens: {");
            for(i = 0; i < num_tokens; i++) {
                printf("%s", tokens[i]);
                if(i < num_tokens - 1) {
                    printf(", ");
                }
            }
            printf("}\n");
        #endif
        status = execute(num_tokens, tokens);
        
        // TODO Write setenv, unsetenv, exit
        // TODO Write execute_binary
        // TODO use .kapishrc to set terminal type (and hopefully more)
                // setenv TERM xxxx seems to be the syntax for this
        // TODO prevent control+c from terminating kapish -> from the looks of it ^C interrupts 
        //      the process, likely just need a handler for this.
        // TODO Implement history cmd and ! functionality
        // TODO change colour of working dir text nad "? " relative to input text from user

        free(input_line);
        if(tokens) {
            free(tokens);
        }
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
    char *input_line = (char *) emalloc(sizeof(char) * buffsize);
    int chars = 0;
    char c;
    do {
        c = getchar();
        if(chars > buffsize - 2) {
            // increase input_line memory
            buffsize = buffsize * 2;
            input_line = (char *) realloc(input_line, buffsize);
            if(!input_line) {
                printf("Realloc Failed for input_line\n");
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
 * of all tokens. Returns null if no tokens are found.
 */
char** tokenize(char *str, int *num_tokens) {
    #define WHITESPACE_DELIM " \n\r\t\a"
    #define MIN_BUF_INCREMENT 10 

    int len = strlen(str);
    int buffer_increment = len/5 >= MIN_BUF_INCREMENT ? len/5 : MIN_BUF_INCREMENT;
    int buffer_size = buffer_increment;

    #ifdef DEBUG
        printf("tokenizing string \'%s\' of length %lu\n", str, strlen(str));
        printf("using buffer increments of size %d\n", buffer_increment);
    #endif

    char **tokens = emalloc(buffer_size * sizeof(char*));
    char *token = strtok(str, WHITESPACE_DELIM);
    int tokens_stored = 0;

    while(token) {
        // store the token, resize buffer if necessary
        if(tokens_stored >= buffer_size) {
            #ifdef DEBUG
                printf("Reallocating. Tokens stored: %d, buffer size: %d\n", tokens_stored, buffer_size);
            #endif
            buffer_size = buffer_size + buffer_increment;
            tokens = realloc(tokens, buffer_size*sizeof(char*));
            if(!tokens) {
                printf("Reallocation Failed during tokenization\n");
                exit(3);
            }
        }
        tokens[tokens_stored++] = token; 
        token = strtok(NULL, WHITESPACE_DELIM);
    }
    // Free up unused memory
    if(tokens_stored) {
        #ifdef DEBUG
            printf("Reallocating to free unused space. Tokens stored: %d\n", tokens_stored);
        #endif
        tokens = realloc(tokens, tokens_stored*sizeof(char *));
        if(!tokens) {
            printf("Reallocation Failed in optimization for token storage\n");
            exit(3);
        }
    } else {
        free(tokens);
        tokens = NULL;  
    }   
    *num_tokens = tokens_stored;
    return tokens;
} 

/*
 * Execute the command given.
 * Return 0 if successful, non-0 otherwise.
 */
int execute(int num_args, char ** args) {
    // TODO check for a builtin, otherwise (try to) execute the specified binary file
    if(0 == num_args || NULL == args || NULL == args[0]) {
        return 0; // No command given
    }
    char* cmd = args[0];
    int i;
    for(i = 0; i < BUILTINS; i++) {
        if(0 == strcmp(cmd, mappings[i].name)) {
            return mappings[i].handler(num_args, args);
        }
    }
    return execute_binary(num_args, args);
}

/* TODO
 * Queries PATH (and ENV Variables?) to find the corresponding binary file
 * Returns 0 if process started successfully 
 */
int execute_binary(int num_args, char **args) {
    printf("Execute binary not implemented yet\n");
    return 0;
}

/* TODO
 * Exit the program
 */
int builtin_exit(int num_args, char **args) {
    printf("Exit not implemented yet\n");
    return 0;
    // Call teardown if required, then exit(0);
}

/*
 * Change working directory to the one specified
 */
int builtin_cd(int num_args, char **args) {
    if(num_args < 2 || NULL == args[1]) {
        printf("Error. No directory specified.\n");
    } else {
        int result = chdir(args[1]);
        if(result) {
            printf("Failed to switch dir to \'%s\'\n", args[1]);
        } else {
            printf("Changed dir to \'%s\'\n", args[1]);
        }
    }
    return 0;
}

/* TODO
 * Create env var if it does not exist 
 * Intialize as specified (or to empty string if no option provided)
 */
int builtin_setenv(int num_args, char **args) {
    printf("Setenv not implemented yet\n");
    return 0;
}

/* TODO
 * Destroy env variable specified
 */
int builtin_unsetenv(int num_args, char **args) {
    printf("Unsetenv not implemented yet\n");
    return 0;
}


/*
 * Error-handling wrapper for malloc
 */
void *emalloc(int size) {
    void *p = malloc(size);
    if(!p) {
        printf("EMalloc Failed. Requested size: %d", size);
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
