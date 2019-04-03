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

#include "file.h"
#include "../disk/vdisk.h"

/*============================FREE LISTS======================================*/
// TODO Ensure changes go to disk eventually
// TODO Ensure that this is loaded correctly

// Reference: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

// bit = 1 if available, 0 if in use
bitvector_t *free_block_list;
bitvector_t *free_inode_list;
// free_block_list.n = 0;
// free_block_list.next_available = 0;

/*
 * Returns true iff block is not being used
 * bit is 0 if in use, 1 otherwise
 */ 
bool test_vector_bit(bitvector_t vector, short index)) {
    unsigned char byte = vector.vector[index/8];
    unsigned char mask = 1; // 0000  0001
    mask = mask << index % 8;
    return byte & mask;
}

/*
 * Mark as in use
 */ 
void clear_vector_bit(bitvector_t vector, short index) {
    unsigned char byte = vector.vector[index/8];
    unsigned char mask = 1; // 0000 0001
    mask = ~(mask << index % 8);
    // printf("mask: %x\n", mask);
    vector.vector[index/8] = byte & mask;
}

/*
 * Mark as available
 */ 
void set_vector_bit(bitvector_t vector, short index) {
    unsigned char byte = vector.vector[index/8];
    unsigned char mask = 1; // 0000 0001 
    mask = mask << index % 8;
    vector.vector[index/8] = byte | mask;
}

/*=================================== INODE MAP ==============================*/
// TODO


/*=================================== LLFS API ===============================*/
/*
 * Wipes and formats a fresh virtual disk. Will do so in the current working 
 * directory if alt_disk is omitted.
 * 
 * This formatting entails the following:
 * 1. Clears the vdisk file
 * 2. Writes the magic number (to identify formatting as LLFS), number of blocks 
 *    on the disk, and the number of inodes which can be allowed to exist
 * 3. Sets up a bitmap for the freelist
 * 4. Sets up reserved blocks to use for the inode map
 * 5. Creates a root directory
 */ 
void initLLFS(FILE *alt_disk) {
    // Clear disk
    void *zeros = calloc(1, BYTES_PER_BLOCK);
    for(int i = 0; i < BLOCKS_ON_DISK; i++){
        vdisk_write(i, zeros, 0, BYTES_PER_BLOCK, alt_disk);
    }

    // Write superblock content
    int sb_content[3] = { MAGIC_NUMBER, BLOCKS_ON_DISK, NUM_INODES };
    vdisk_write(0, sb_content, 0, sizeof(int) * 3, alt_disk);

    // Write free list bit vector using the buffer declared in the free list 
    for(short i = 0; i < 10; i++) {
        clear_free_list_bit(i); // First 10 blocks are reserved 
    }
    for(short i = 10; i < BLOCKS_ON_DISK; i++) {
        set_free_list_bit(i);
    }
    vdisk_write(1, free_list, 0, BYTES_PER_BLOCK, alt_disk);

    // TODO Write inode map/table/etc


    // TODO Set up root dir

}

// TODO Read and write functionality

// TODO create segment buffer to be sent to disk when full 
// Need a way to ensure this is written on process end
    // One way to do this is to put the onus on the user to perform a manual
    // checkpoint before ending a program which uses this library 
    // (essentially like closing a file system) 

// In addition to writing out the segment, we will want to write updates made to
// the free list and inode map since last checkpoint