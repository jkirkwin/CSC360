/* Author: Jamie Kirkwin
 *
 * Tests for the disk API given in vdisk.c/vdisk.h and internally used helpers
 * 
 * TODO: this should be re-written with a framewordk like cunit, but cunit is
 *       unsupported for windows and I don't want to have to test exclusively on
 *       the linux server so we're going to just do ad hoc tests 
 */ 

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vdisk.h"

#define NUM_TESTS 5

bool test_get_vdisk_path(); // Unimplemented
bool test_read_default_location(); // Unimplemented
bool test_read_custom_location(); // Unimplemented
bool test_write_default_location(); // Unimplemented
bool test_write_custom_location(); // Unimplemented

/* 
 * Manually writes blocks 0, 10, and 4095 of the specified vdisk with a's
 * Writes the vdisk in the currrent dir if none is specified
 */ 
void manual_write(char *disk_path) {
    FILE *file;
    if(disk_path) {
        file = fopen(disk_path, "wb");
    } else {
        file = fopen("./vdisk", "wb");
    }

    char data[1] =  {'a'};
    int i;
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        fwrite(data, sizeof(char), 1, file);
    }
    if(fseek(file, BYTES_PER_BLOCK * 10, SEEK_SET)) {
        fprintf(stderr, "Seek 1 failed in manual write\n");
    }
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        fwrite(data, sizeof(char), 1, file);
    }
    if(fseek(file, BYTES_PER_BLOCK * 4095, SEEK_SET)) {
        fprintf(stderr, "Seek 2 failed in manual write\n");
    }
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        fwrite(data, sizeof(char), 1, file);
    }
    fclose(file);
}

//==================DANGER====================
// /*
//  * Reads the whole file and returns its contents
//  */ 
// void *manual_read(char *disk_path) {
//     FILE *file;
//     if(disk_path) {
//         file = fopen(disk_path, "wb");
//     } else {
//         file = fopen("./vdisk", "wb");
//     }
//     char *content = (char *) malloc(BLOCKS_ON_DISK * BYTES_PER_BLOCK + 1);
//     if(!content) {
//         fprintf(stderr, "malloc failed in manual read\n");
//         return NULL;
//     }
//     char ch;
//     int i = 0;
//     while((ch = fgetc(file)) != EOF) {
//         printf("%c\n", ch);
//         content[i++] = ch;
//     }
//     content[i] = '\0';
//     return content;
// }

int main(int argc, char **argv) {
    char * content = manual_read(NULL);
    if(!content) printf("NULL\n");
    printf("%s\n", content);

    bool (*tests[NUM_TESTS]) ();
    tests[0] = test_get_vdisk_path;
    tests[1] = test_read_default_location;
    tests[2] = test_read_custom_location;
    tests[3] = test_write_default_location;
    tests[4] = test_write_custom_location;
    
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

bool test_read_default_location() {
    manual_write();
    char *buffer = (char *) malloc(BYTES_PER_BLOCK); 
    vdisk_read(0, buffer, NULL);
    int i;
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            return false;
        }
    }
    vdisk_read(10, buffer, NULL);
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            return false;
        }
    }
    vdisk_read(4095, buffer, NULL);
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            return false;
        }
    }
    return true;

}

bool test_read_custom_location() {
    // TODO
    return false;

}

bool test_write_default_location() {
    // TODO
    return false;

}

bool test_write_custom_location() {
    // TODO
    return false;

}
