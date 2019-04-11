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

#define NUM_TESTS 4

bool test_read_default_location(); 
bool test_read_custom_location(); 
bool test_write_default_location(); 
bool test_write_custom_location(); 

char *test_names[4] = {
    "test_read_default_location",
    "test_read_custom_location",
    "test_write_default_location",
    "test_write_custom_location"
};

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

// A helper to zero out the vdisk
void clear_disk(char *disk_path) {
    FILE *file;
    if(disk_path) {
        file = fopen(disk_path, "wb");
    } else {
        file = fopen("./vdisk", "wb");
    }
    void *zeros = calloc(1, BYTES_PER_BLOCK);
    fwrite(zeros, BYTES_PER_BLOCK, BLOCKS_ON_DISK, file);
    fclose(file);
}

int main(int argc, char **argv) {
    printf("\nRunning tests for Disk\n");

    bool (*tests[NUM_TESTS]) ();
    tests[0] = test_read_default_location;
    tests[1] = test_read_custom_location;
    tests[2] = test_write_default_location;
    tests[3] = test_write_custom_location;
    
    int passed = 0, failed = 0;
    for(int i = 0; i < NUM_TESTS; i++) {
        printf("%d.  %s: ", i+1, test_names[i]);
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

bool test_read_default_location() {
    clear_disk(NULL);
    manual_write(NULL);
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
    clear_disk("./vdisk");
    manual_write("./vdisk");

    char *buffer = (char *) malloc(BYTES_PER_BLOCK);
    FILE* fp = fopen("./vdisk", "rb"); 
    vdisk_read(0, buffer, fp);
    int i;
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            fclose(fp);
            return false;
        }
    }
    vdisk_read(10, buffer, fp);
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            fclose(fp);
            return false;
        }
    }
    vdisk_read(4095, buffer, fp);
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(buffer[i] != 'a') {
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

/* 
 * Write blocks 0, 50 (with offset), 4095
 */ 
bool test_write_default_location() {
    clear_disk(NULL);
    char *content = (char *) calloc(1, BYTES_PER_BLOCK);
    int i;
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        content[i] = 'q';
    }
    vdisk_write(0, content, 0, BYTES_PER_BLOCK, NULL);
    vdisk_write(50, content, 10, BYTES_PER_BLOCK - 10, NULL);
    vdisk_write(4095, content, 0, BYTES_PER_BLOCK, NULL);

    FILE *fp = fopen("./vdisk", "rb");
    char *file_content = (char *) calloc(1, BYTES_PER_BLOCK * BLOCKS_ON_DISK);
    fread(file_content, BYTES_PER_BLOCK * BLOCKS_ON_DISK, 1, fp);

    fclose(fp);

    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(file_content[i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 0\n");
            free(file_content);
            return false;
        }
        if(file_content[BYTES_PER_BLOCK * 4095 + i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 4095\n");
            free(file_content);
            return false;
        }
        if(i < 10 && file_content[BYTES_PER_BLOCK * 50 + i] == 'q') {
            fprintf(stderr, "\tincorrect 'q' in block 50\n");
            free(file_content);
            return false;
        } else if(i >= 10 && file_content[BYTES_PER_BLOCK * 50 + i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 50\n");
            free(file_content);
            return false;
        }
    }
    if(file_content[i+1] == 'q') {
        free(file_content);
        return false;
    }

    free(file_content);
    return true;
}

bool test_write_custom_location() {
    clear_disk("./vdisk");
    char *content = (char *) calloc(1, BYTES_PER_BLOCK);
    int i;
    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        content[i] = 'q';
    }
    FILE *fp = fopen("./vdisk", "rb+");
    vdisk_write(0, content, 0, BYTES_PER_BLOCK, fp);
    vdisk_write(50, content, 10, BYTES_PER_BLOCK - 10, fp);
    vdisk_write(4095, content, 0, BYTES_PER_BLOCK, fp);
    fclose(fp);
    char *file_content = (char *) calloc(1, BYTES_PER_BLOCK * BLOCKS_ON_DISK);
    fp = fopen("./vdisk", "rb");
    fread(file_content, BYTES_PER_BLOCK * BLOCKS_ON_DISK, 1, fp);
    fclose(fp);

    for(i = 0; i < BYTES_PER_BLOCK; i++) {
        if(file_content[i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 0\n");
            free(file_content);
            return false;
        }
        if(file_content[BYTES_PER_BLOCK * 4095 + i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 4095\n");
            free(file_content);
            return false;
        }
        if(i < 10 && file_content[BYTES_PER_BLOCK * 50 + i] == 'q') {
            fprintf(stderr, "\tincorrect 'q' in block 50\n");
            free(file_content);
            return false;
        } else if(i >= 10 && file_content[BYTES_PER_BLOCK * 50 + i] != 'q') {
            fprintf(stderr, "\tmissing 'q' in block 50\n");
            free(file_content);
            return false;
        }
    }
    if(file_content[i+1] == 'q') {
        free(file_content);
        fprintf(stderr, "\tincorrect q in block 1 (clobbered from block 0 write)\n");
        return false;
    }
    free(file_content);
    return true;
}