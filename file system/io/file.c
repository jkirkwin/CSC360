/*
 * Jamie Kirkwin
 * V00875987
 * March 2019
 * CSC 360 Assignment 3
 * 
 * A "Little Log File System" (LLFS)
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../disk/vdisk.h"
#include "file.h"

/*============================FREE LIST=======================================*/
// TODO Ensure changes go to disk eventually
// TODO Ensure that this is loaded correctly

// Inspiration for free list implementation drawn from 
// http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
unsigned char free_list[512]; // A bit map showing whether a given block is free

/*
 * Returns true iff block is not being used
 * bit is 0 if in use, 1 otherwise
 */ 
bool test_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000  0001
    mask = mask << block_num % 8;
    return byte & mask;
}

/*
 * Mark as in use
 */ 
void clear_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000 0001
    mask = !(mask << block_num % 8);
    printf("mask: %d\n", mask);
    free_list[block_num/8] = byte & mask;
}

/*
 * Mark as available
 */ 
void set_free_list_bit(short block_num) {
    unsigned char byte = free_list[block_num/8];
    unsigned char mask = 1; // 0000 0001 
    mask = mask << block_num % 8;
    free_list[block_num/8] = byte | mask;
}

/*=================================== INODE MAP ==============================*/




/*=================================== LLFS API ===============================*/
void initLLFS(FILE *alt_disk) {
    // Clear disk
    void *zeros = calloc(1, BYTES_PER_BLOCK);
    for(int i = 0; i < BLOCKS_ON_DISK; i++){
        vdisk_write(i, zeros, 0, BYTES_PER_BLOCK, alt_disk);
    }

    // Write superblock content
    int sb_content[3] = { MAGIC_NUMBER, BLOCKS_ON_DISK, NUM_INODES };
    vdisk_write(0, sb_content, 0, sizeof(int) * 3, alt_disk);

    // Write free list bit vector using the buffer declared in the free list section
    for(short i = 0; i < 10; i++) {
        clear_free_list_bit(i); // First 10 blocks are reserved
    }
    for(short i = 10; i < BLOCKS_ON_DISK; i++) {
        set_free_list_bit(i);
    }

    // TODO Write inode map/table/etc

}