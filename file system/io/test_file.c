/*
 * Unit tests for file system library
 */ 


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "file.h"
#include "../disk/vdisk.h"

#define NUM_TESTS 1

// Test method declarations go here
bool test_free_list_api();

char *test_names[NUM_TESTS] = {
    "test_free_list_api"
};

// Helper methods go here


int main(int argc, char **argv) {
    bool (*tests[NUM_TESTS]) ();
    tests[0] = test_free_list_api;
    
    int passed = 0, failed = 0;
    for(int i = 0; i < NUM_TESTS; i++) {
        printf("%s: ", test_names[i]);
        if(tests[i]()) {
            passed++;
            printf("PASSED\n");
        } else {
            failed++;
            printf("FAILED\n");
        }
    }
    printf("\nPassed %d/%d tests\n", passed, NUM_TESTS);
}

// Tests go here
bool test_free_list_api() {
    short blocks_to_test[4] = {0,1,1000,4095};
    for(int i = 0; i < 4; i++) {
        clear_free_list_bit(blocks_to_test[i]);
        if(test_free_list_bit(blocks_to_test[i])) {
            return false;
        }
        set_free_list_bit(blocks_to_test[i]);
        if(!test_free_list_bit(blocks_to_test[i])) {
            return false;
        }
    }
    return true;
}