#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "history.h"

int test_hist_pop();
int test_hist_push();
int test_hist_get();
int test_hist_size();
int test_init_hist();
int test_clear_hist();
int test_match_prefix();

char **getwords(int num_words);
void setup(int num_elements, char **elements);
void set_error(char *);

#define BUFFSIZE 100
char error_buffer[BUFFSIZE];

#define NUM_TESTS 7
int (*test_functions[]) () = {
    &test_hist_pop,
    &test_hist_push,
    &test_hist_get,
    &test_hist_size,
    &test_init_hist,
    &test_clear_hist,
    &test_match_prefix
};

int main() {
    printf("Running History Tests\n");
    printf("====================\n\n");
    int tests_passed = 0;
    int i;
    for(i = 0; i < BUFFSIZE; i++) {
        error_buffer[i] = '\0';
    }
    for(i = 0; i < NUM_TESTS; i++) {
        if(test_functions[i]()) {
            printf("FAIL: %s\n", error_buffer);
        } else {
            printf("PASS\n");
            tests_passed++;
        }
    }
    printf("Suite Completed. %d/%d Tests Passed.\n", tests_passed, NUM_TESTS);
}

void set_error(char *msg) {
    int len = strlen(msg);
    if(len >= BUFFSIZE) {
     len = BUFFSIZE-1;
    }
    strncpy(error_buffer, msg, len);
    error_buffer[len] = '\0';   
}

char **getwords(int num_words) {
    char **words = (char**) malloc(sizeof(char*) * num_words);
    if(!words) {
        perror("Malloc failed: ");
        exit(1);
    }
    char *word;
    int i;
    for(i = 0; i < num_words; i++) {
        word = (char*) malloc(sizeof(char) * 30);
        sprintf(word, "word%d", i);
        words[i] = word;
    }
    return words;
}

void setup(int num_elements, char **elements) {
    // uninitialize
    clear_hist();

    // re-init
    init_hist();

    // insert elements with push()
    int i;
    for(i = 0; i < num_elements; i++) {
        hist_push(elements[i]);
    }
}


int test_hist_pop() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with 1 item
    //      List with many items
    return -1;
}



int test_hist_push() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with some items
    //      List with max items
    return -1;
}


int test_hist_get() {
    // Cases:
    //      Uninitialized
    //      List with some items
    return -1;
}


int test_hist_size() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with 1 item
    //      List with some items
    clear_hist();
    if(hist_size() != 0) {
        set_error("uninitialized list size not 0");
        return 1;
    } 
    init_hist();
    if(hist_size() != 0) {
        set_error("empty list size not 0");
        return 1;
    } 
    char *singleton = "t h s";
    setup(1, &singleton); 
    if(hist_size() != 1) {
        set_error("single item list size not 1");
        return 1;
    } 
    char **words = getwords(100);
    setup(100, words);
    if(hist_size() != 100) {
        set_error("list of size 100 incorrect size");
        return 1;
    } 
    return 0;
}


int test_init_hist() {
    // Cases:
    //      Uninitialized
    //      Initialized
    return -1;
}


int test_clear_hist() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with some items
    return -1;
}


int test_match_prefix() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with 1 item
    //      List with some items
    //          1 Match
    //          Multiple matches
    //          No matches
    return -1;
}
