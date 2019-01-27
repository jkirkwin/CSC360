
/*
 * A white box test suite for Kapish's history stack 
 * (independant of the shell its self)
 */

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

char *func_names [] = {
    "pop",
    "push",
    "get",
    "size",
    "init_hist",
    "clear",
    "match_prefix"  
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
        printf("Testing %s: ", func_names[i]);
        set_error("No message given");
        if(test_functions[i]()) {
            printf("FAILED: %s\n", error_buffer);
        } else {
            printf("PASSED\n");
            tests_passed++;
        }
    }
    printf("\nSuite Completed. %d/%d Tests Passed.\n", tests_passed, NUM_TESTS);
}

/*
 * Insert msg into error buffer
 */
void set_error(char *msg) {
    int len = strlen(msg);
    if(len >= BUFFSIZE) {
     len = BUFFSIZE-1;
    }
    strncpy(error_buffer, msg, len);
    error_buffer[len] = '\0';   
}

/*
 * Make and return a 2d array of strings
 */ 
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

/*
 * Clear the stack, re-init, and push in the given strings
 */ 
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

    char oracle[] = "oracle\0";
    char *result;
    clear_hist();

    result = hist_pop();
    if(result) {
        set_error("Uninitialized pop allowed");
        return -1;
    }
    
    init_hist();
    result = hist_pop();
    if(result) {
        set_error("Pop allowed on empty stack");
        return -1;
    }
    
    hist_push(oracle);
    result = hist_pop();
    if(!result) {
        set_error("Pop failed (size 1)");
        return -1;
    } else if(0 != strncmp(oracle, result, strlen(oracle))) {
        set_error("Pop return incorrect (size 1)");
        return -1;
    } else if(hist_size() != 0) {
        set_error("Pop does not decrement size (size 0)");
        return -1;
    }
    
    setup(100, getwords(100));
    hist_push(oracle);
    result = hist_pop();
    if(!result) {
        set_error("Pop failed (size 100)");
        return -1;
    } else if(0 != strncmp(oracle, result, strlen(oracle))) {
        set_error("Pop return incorrect (size 100)");
        return -1;
    } else if(hist_size() != 100) {
        set_error("Pop does not decrement size (size 100)");
        return -1;
    }

    return 0;
}


int test_hist_push() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with some items
    //      List with max items
    char oracle[] = "oracle\0";
    char *result;

    clear_hist();
    hist_push(oracle);
    result = hist_get(0);
    if(!result || 0 != strncmp(oracle, result, strlen(oracle)) || hist_size() != 1) { // strncmp seemingly fails
        set_error("Uninitialized push failed");
        return -1;
    }

    clear_hist();
    setup(0, NULL);
    hist_push(oracle);
    if(0 != strncmp(oracle, hist_get(0), strlen(oracle)) || hist_size() != 1) {
        set_error("Uninitialized push failed");
        return -1;
    }

    clear_hist();
    setup(2, getwords(2));
    hist_push(oracle);
    if(0 != strncmp(oracle, hist_get(0), strlen(oracle)) || hist_size() != 3) {
        set_error("Uninitialized push failed");
        return -1;
    }

    clear_hist();
    setup(24, getwords(24));
    hist_push(oracle);
    if(0 != strncmp(oracle, hist_get(0), strlen(oracle)) || hist_size() != 25) {
        set_error("Uninitialized push failed");
        return -1;
    }

    return 0;
}


int test_hist_get() {
    // Cases:
    //      Uninitialized
    //      List with many items
    
    clear_hist();
    if(hist_get(0)) {
        set_error("Uninitialized list");
    }

    int num_words = 100;
    char **words = getwords(num_words);
    setup(num_words, words);
    int i;
    for(i = 0; i < num_words; i++) {
        if(strncmp(hist_get(i), words[num_words-1-i], strlen(words[num_words-1-i])) != 0) {
            set_error("Comparison failed");
            return -1;
        }
    }

    return 0;
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

    clear_hist();
    if(is_initialized() || hist_size() != 0) {
        return -1;
    }
    init_hist();
    if(!is_initialized()) {
        return -1;
    }
    init_hist();
    if(!is_initialized()) {
        return -1;
    }
    return 0;
}


int test_clear_hist() {
    // Cases:
    //      Uninitialized
    //      Empty list
    //      List with some items

    clear_hist();
    if(is_initialized() || hist_size() != 0) {
        return -1;
    }
    init_hist();
    clear_hist();
    if(is_initialized() || hist_size() != 0) {
        return -1;
    }
    setup(100, getwords(100));
    clear_hist();
    if(is_initialized() || hist_size() != 0) {
        return -1;
    }
    return 0;
}


int test_match_prefix() {
    // Cases:
    //      NULL prefix
    //      Empty list
    //      List with 1 item
    //          No match 
    //          Match
    //      List with some items
    //          1 Match
    //          Multiple matches
    //          No matches
    char prefix[] = "pre";

    setup(10, getwords(10));
    if(match_prefix(NULL) != -1) {
        set_error("Null prefix");
        return -1;
    }

    setup(0, NULL);
    if(match_prefix(prefix) != -1) {
        set_error("Empty list");
        return -1;
    }

    clear_hist();
    init_hist();
    hist_push("prefix abcdefg");
    if(match_prefix(prefix) != 0) {
        set_error("Size 1, expected match");
        return -1;
    }
    if(match_prefix("xxxx") != -1) {
        set_error("Size 1, expected mismatch");
        return -1;
    }

    setup(20, getwords(20));
    if(match_prefix(prefix) != -1) {
        set_error("Lg list, bad match");
        return -1;
    }
    hist_push("prefix 123456");
    if(match_prefix(prefix) != 0) {
        set_error("Lg list, wrong index returned (single)");
        return -1;
    }
    if(match_prefix("word1") != 1) { // should match word19
        set_error("Lg list, wrong index returned (multiple)");
        return -1;
    }

    return 0;
}
