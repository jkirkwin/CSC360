// Functionality
void init();
int main_loop();
char * get_input_line();
char** tokenize(char *str, int *num_tokens);
int execute(int num_args, char ** args);
int execute_binary(int num_args, char **args);
int builtin_exit(int num_args, char **args);
int builtin_cd(int num_args, char **args);
int builtin_setenv(int num_args, char **args);
int builtin_unsetenv(int num_args, char **args);
int builtin_history(int num_args, char **args);
void sig_handler(int);

// Utility
void *emalloc(int size);
void chop(char*); 

// Debugging
void print_hex(char*);
void print_isspace(char *);