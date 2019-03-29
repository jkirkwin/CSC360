/* Author: Jamie Kirkwin
 *
 * Tests for the disk API given in vdisk.c/vdisk.h and internally used helpers
 * 
 * TODO: this should be re-written with a framewordk like cunit, but cunit is
 *       unsupported for windows and I don't want to have to test exclusively on
 *       the linux server so we're going to just do ad hoc tests 
 */ 

#include "vdisk.h"
#include <stdbool.h>
#include <stdio.h>

#define NUM_TESTS 5

bool test_get_vdisk_path();
bool test_read_cwd();
bool test_read_custom_path();
bool test_write_cwd();
bool test_write_custom_path();

/* 
 * Manually writes blocks 0, 10, and 4095 of the specified vdisk with 1's
 * Writes the vdisk in the currrent dir if none is specified
 */ 
void manual_write(char *disk_path) {
    // TODO
    FILE *file;
    if(disk_path) {
        file = fopen(disk_path, 'w');
    } else {
        file = fopen("./vdisk", 'w');
    }
    int i;
    for(i = 0; i < )
}

void manual_read() {

}

int main(int argc, char **argv) {
    bool (*tests[NUM_TESTS]) ();
    tests[0] = test_get_vdisk_path();
    tests[1] = test_read_cwd();
    tests[2] = test_read_custom_path();
    tests[3] = test_write_cwd();
    tests[4] = test_write_custom_path();
    
    int passed = 0, failed = 0;
    for(int i = 0; i < NUM_TESTS; i++) {
        printf("Test %d: ", i);
        if(tests[i]()) {
            passed++;
            printf("passed\n");
        } else {
            failed++;
            printf("failed\n");
        }
    }
    printf("Passed %d/%d tests\n", passed, NUM_TESTS);
}

bool test_get_vdisk_path() {
    // TODO
    return false;

}

bool test_read_cwd() {
    // TODO
    // Manually write 3 blocks of 1's to the disk and verify that they 

    

    return false;

}

bool test_read_custom_path() {
    // TODO
    return false;

}

bool test_write_cwd() {
    // TODO
    return false;

}

bool test_write_custom_path() {
    // TODO
    return false;

}
