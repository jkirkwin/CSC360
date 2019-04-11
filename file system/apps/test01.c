#include <stdio.h>
#include <stdlb.h>

#include "../disk/vdisk.h"

/*
 * As per the spec, this file showcases the functionality of reading and writing
 * to the disk. For the unit tests, see disk/test_vdisk.c
 */ 

int main() {
    printf("Test1\n");
    // Fork a process that writes to a vdisk in the current directory
    // Wait for it to finish
    // read from the vdisk and make sure that the content is the same.
    
    printf("Creating oracle string to write and read back from the disk\n");
    char oracle[512]; 
    char result[512]; 
    char c = 'a'
    for(int i = 0; i < 512; i++) {
        oracle[i] = c++;
    }
    short block_num = 100;

    printf("Forking child process to write to the disk\n");
    int pid = fork();
    if(pid < 0) {
        fprintf(stderr, "Fork failed. Cannot run test\n");
        exit(1);
    }
    if(0 == pid) {
        // Child will write to the disk
        printf("Child process writing to disk in current directory\n");
        if(vdisk_write(block_num, oracle, 0, 512, NULL)) {
            printf("Write successful\n");
        } else {
            printf("Write failed\n");
            exit(1);
        }
    } else {
        // Store child id and wait for it to terminate
        wait(NULL);
        printf("Child terminated, parent will now read back the content from disk\n");
        if(vdisk_read(block_num, result, NULL)) {
            printf("Read successful, comparing value read to value written\n");
            for(int i = 0; i < 512; i++) {
                if(result[i] != oracle[i]) {
                    printf("Mismatch - test failed\n");
                    exit(1);
                }
            }
            printf("Success! Content matches!\nTest Complete.\n");
        } else {
            printf("Read failed\n");
            exit(1);
        }
    }

}
