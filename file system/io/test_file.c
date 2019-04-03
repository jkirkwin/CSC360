/*
 * Unit tests for file system library file.c/file.h
 * Also requires the virtual disk api from /disk/
 */ 

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "file.h"
#include "../disk/vdisk.h"

#define NUM_TESTS 2

// ====  ==== ==== ====  Test method declarations go here ==== ==== ====
bool test_free_list_api();
bool test_init_LLFS();

char *test_names[NUM_TESTS] = {
    "test_free_list_api",
    "test_init_LLFS"
};

// ==== ==== ==== ==== Helper methods go here ==== ==== ==== ==== ==== 


int main(int argc, char **argv) {
    printf("\nRunning tests for LLFS\n");
    
    bool (*tests[NUM_TESTS]) ();
    tests[0] = test_free_list_api;
    tests[1] = test_init_LLFS;

    
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

// ==== ==== ==== ==== Tests go here ==== ==== ==== ==== ==== 
bool test_free_list_api() {
    short blocks_to_test[4] = {0,1,1000,4095};
    bitvector_t vect;
    vect.n = 0;
    vect.next_available = 0; 
    // unsigned char free_list[BYTES_PER_BLOCK];
    for(int i = 0; i < 4; i++) {
        clear_vector_bit(vect, blocks_to_test[i]);
        if(test_vector_bit(vect, blocks_to_test[i])) {
            return false;
        }
        set_vector_bit(vect, blocks_to_test[i]);
        if(!test_vector_bit(vect, blocks_to_test[i])) {
            return false;
        }
    }
    return true;
}

bool test_init_LLFS() {
    // TODO
    return false;
}