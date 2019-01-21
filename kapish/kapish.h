// Functionality
int main_loop();
char * get_input_line();
char** tokenize(char *str, int *num_tokens);

// Utility
void *emalloc(int size);
void chop(char*); 

// Debugging
void print_hex(char*);
void print_isspace(char *);