/*
 * Jamie Kirkwin
 * jkirkwin 
 * V00875987
 * CSC360 Assignment 1, Kapish Shell
 * Modified Jan 27, 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // isspace
#include <unistd.h> // chdir 
#include <sys/wait.h> // wait
#include <signal.h> // signal handling

#include "kapish.h"
#include "history.h"

#define BUILTINS 5

typedef struct mapping {
    char *name;
    int (*handler)(int, char**);
} mapping_t;

mapping_t mappings[] = {
    {"cd\0", &builtin_cd},
    {"exit\0", &builtin_exit},
    {"setenv\0", &builtin_setenv},
    {"unsetenv\0", &builtin_unsetenv},
    {"history\0", &builtin_history}
};

int cid = 0; // child id
int interrupted = 0;

int main(int argc, char const *argv[]) {
    #ifdef DEBUG
        printf("Running in DEBUG mode\n");
        printf("=====================\n\n");
    #endif
    init();
    main_loop();
    terminate();
    return 0;
}

/*
 * Loads the config file .kapishrc from the users home 
 * directory and sets up the shell
 * 
 * Initializes command history data structure
 */ 
void init() {
    #define CONFIG_NAME ".kapishrc"
    #define HOME getenv("HOME")

    init_hist();

    // Set ctrl-c handler
    signal(SIGINT, sig_handler);

    // Build filename string
    int len = strlen(CONFIG_NAME) + strlen(HOME) + 2;
    char *filename = emalloc(sizeof(char) * len); 
    strncpy(filename, HOME, len-2);
    strncat(filename, "/", len-2);
    strncat(filename, CONFIG_NAME, strlen(CONFIG_NAME));
    filename[len-1] = '\0';

    FILE *fileptr = fopen(filename, "r");
    if(NULL == fileptr) {
        printf("ERROR: Cannot find configuration file in user's home directory.\n");
        return;
    }

    int eof_flag = 0;
    char *config_line = get_input_line(&eof_flag, fileptr);
    char ** tokens;
    int num_tokens = 0;
    while(0 == eof_flag) {
        printf("> %s\n", config_line);
        tokens = tokenize(config_line, &num_tokens);
        execute(num_tokens, tokens);
        free(tokens);
        free(config_line);
        config_line = get_input_line(&eof_flag, fileptr);
    }
    if(filename) {
        free(filename);
    }
    if(config_line) {
        free(config_line);
    }
    if(fileptr) {
        free(fileptr);
    }
    printf("Config Complete\n");
    printf("===============\n");
}

/*
 * Kills child process if interrupt signal is given
 */ 
void sig_handler(int s) {
    signal(s, SIG_IGN); // Ignore signal for the duration of the handler
    if(s == SIGINT && cid > 0) {
        interrupted = 1; // Allows other code to handle the interruption
        kill(cid, SIGKILL);
        cid = 0;
    }
    signal(s, sig_handler); // Re-instate handler
}

/*
 * Loop until user exits or system crashes (which shouldn't happen)
 */ 
int main_loop() {
    #ifdef DEBUG
        printf("Main loop entered\n");
    #endif
    int eof_flag;
    int status = 0;
    char *input_line;
    char **tokens;
    int num_tokens;
    while(0 == status) {
        printf("? ");
        eof_flag = 0;
        input_line = get_input_line(&eof_flag, stdin); 
        if(eof_flag) {
            if(input_line) {
                free(input_line);
            }
            return 1;
        }
        if(input_line && strlen(input_line) > 0) {
            // Check for shebang and log the command in history stack
            if(input_line[0] == '!') {
                // Push the command that matches if there is one. 
                // Push the command entered otherwise.
                int match = match_prefix(strlen(input_line) > 1 ? &input_line[1] : "");
                if(match > -1) {
                    free(input_line);
                    input_line = hist_get(match);
                    printf("%s\n", input_line);
                }
            }
            hist_push(input_line);
        } 
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
 * Get a line of input character-by-character to ensure we don't chop off the 
 * end of the input as we might when using a buffer.
 * Strips any trailing whitespace from the string before returning it.
 * Sets the status flag if EOF was encountered, clears it if not.
 */
char* get_input_line(int *eof_flag, FILE *file) {
    if(!file) {
        printf("No file speicified\n");
        return NULL;
    }
    #ifdef DEBUG
        printf("getting input line\n");
    #endif
    int buffsize = 1000;
    char *input_line = (char *) emalloc(sizeof(char) * buffsize);
    int chars = 0;
    char c;
    do {
        c = fgetc(file);
        if(chars > buffsize - 2) {
            // increase input_line capacity
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
    if(c == EOF) {
        *eof_flag = 1;
    } else {
        *eof_flag = 0;
    }
    *(input_line+chars) = '\0';
    chop(input_line);
    #ifdef DEBUG
        printf("input line retrieved: %s\n", input_line);
    #endif
    return input_line;
}

/*
 * Tokenizes the string passed in, stores the number of tokens generated, and returns an array
 * of all tokens. Returns null if no tokens are found. Null terminates the array of tokens.
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
    // Free up unused memory and null terminate token array
    if(tokens_stored) {
        #ifdef DEBUG
            printf("Reallocating to free unused space. Tokens stored: %d\n", tokens_stored);
        #endif
        tokens = realloc(tokens, (tokens_stored+1)*sizeof(char *)); 
        if(!tokens) {
            printf("Reallocation Failed in optimization for token storage\n");
            exit(3);
        }
        tokens[tokens_stored] = NULL;
    } else {
        free(tokens);
        tokens = NULL;  
    }
    *num_tokens = tokens_stored;
    return tokens;
} 

/*
 * Execute the command given.
 * Checks for a built-in function matching args[0], otherwise (trys to) execute 
 * args[0] as a binary file via PATH search (actually done by execvp).
 * Return non-0 on fatal error/exit command.
 */
int execute(int num_args, char ** args) {
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

/* 
 * Queries PATH (via execvp) to find the corresponding binary file
 * and runs that command with any args specified
 */
int execute_binary(int num_args, char **args) {
    #ifdef DEBUG
        printf("Attempting to execute process \'%s\'\n", args[0]);
    #endif
    int pid = fork();
    if(pid < 0) {
        perror("Fork failed");
        return 0;
    }
    if(0 == pid) {
        execvp(args[0], &args[0]);
        perror("Execution failed");
        exit(1);
    } else {
        // Store child id and wait for it to terminate
        cid = pid; 
        wait(NULL);
        if(interrupted) {
            printf("\n");
            interrupted = 0;
        }
        cid = 0;
    }
    return 0;
}

/* 
 * Exit the shell
 */
int builtin_exit(int num_args, char **args) {
    printf("\n");
    return 1; // signal to break the main loop and terminate
}

/*
 * Change working directory to the one specified
 */
int builtin_cd(int num_args, char **args) {
    if(1 == num_args || NULL == args[1] || strncmp("~\0", args[1], strlen(args[1])) == 0) {
        char *homedir = getenv("HOME");
        int status = chdir(homedir);
        if(0 == status) {
            printf("%s\n", homedir);
        } else {
            perror("Failed to change to Home dir");
        }
    } else {
        int status = chdir(args[1]);
        if(status) {
            perror("Failed to switch dir");
        } else {
            int max_path_len = 200;
            char pathbuff[max_path_len + 1];
            getcwd(pathbuff, max_path_len); 
            printf("%s\n", pathbuff);
        }
    }
    return 0;
}

/* 
 * Create env var if it does not exist 
 * Initialize as specified (or to empty string if no option provided)
 */
int builtin_setenv(int num_args, char **args) {
    int status = -1;
    if(num_args < 2) {
        printf("No evironment variable specified\n");
        return 0;
    } else if(num_args < 3) {
        status = setenv(args[1], "", 1);
    } else {
        status = setenv(args[1], args[2], 1);
    }
    if(!status) {
        printf("%s=%s\n", args[1], getenv(args[1]));
    } else {
        printf("Failed to sent environment variable \'%s\'\n", args[1]);
    }
    return 0;
}

/* 
 * Destroy env variable specified
 */
int builtin_unsetenv(int num_args, char **args) {
    if(num_args < 2) {
        printf("No evironment variable specified\n");
    } else {
        int status = unsetenv(args[1]);
        if(!status) {
            printf("Environment variable \'%s\' unset\n", args[1]);
        } else {
            printf("Failed to remove environment variable \'%s\'\n", args[1]);
        }
    }
    return 0;
}

/* 
 * Prints all commands entered into the shell in the order they 
 * were entered 
 */
int builtin_history(int num_args, char **args) {
    init_hist(); // for safety
    printf("History:\n");
    char *p;
    int i;
    for(i = hist_size()-1; i >= 0; i--) {
        if((p = hist_get(i))) {
            printf(">>> %s\n", p);
        } else {
            #ifdef DEBUG
                printf("No command stored at index %d\n", i);
            #endif
        }
    }
    return 0;
}


/*
 * Free memory and kill child processes
 */ 
void terminate() {
    if(cid > 0) {
        kill(cid, SIGKILL);
    }
    clear_hist();
    printf("\nGoodbye\n");
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
