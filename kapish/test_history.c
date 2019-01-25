#include <stdlib.h>
#include <stdio.h>

#include "history.h"

int test_hist_pop();
int test_hist_push();
int test_hist_get();
int test_hist_size();
int test_init_hist();
int test_clear_hist();
int test_match_prefix();

void uninitialize();
void setup(int num_elements, char **elements);

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


void uninitialize() {
    // TODO clear the list
}


void setup(int num_elements, char **elements) {
    // uninitialize 
    // re-init
    // insert elements with push()
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
    return -1;
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