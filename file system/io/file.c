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

/*============================FREE LISTS======================================*/
// TODO Ensure changes go to disk eventually
// TODO Ensure that this is loaded correctly

// Reference: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

// bit = 1 if available, 0 if in use
bitvector_t *free_block_list;
bitvector_t *free_inode_list;

/*
 * Returns true iff item is available for use
 * bit is 0 if in use, 1 otherwise
 */
bool test_vector_bit(bitvector_t *vector, short index) {
    unsigned char byte = vector->vector[index/8];
    unsigned char mask = 1; // 0000  0001
    mask = mask << index % 8;
    return byte & mask;
}

/*
 * Mark as in use. Updates n BUT NOT next_available.
 */ 
void clear_vector_bit(bitvector_t *vector, short index) {
    unsigned char byte = vector->vector[index/8];
    unsigned char mask = 1; // 0000 0001
    mask = ~(mask << index % 8);
    vector->vector[index/8] = byte & mask;
    vector->n++; // Update n
}

/*
 * Mark as available. Updates n BUT NOT next_available.
 */ 
void set_vector_bit(bitvector_t *vector, short index) {
    unsigned char byte = vector->vector[index/8];
    unsigned char mask = 1; // 0000 0001
    mask = mask << index % 8;
    vector->vector[index/8] = byte | mask;
    vector->n--;
}

/*============================ INODES AND IMAP ==============================*/
// TODO

// Maps inode block numbers to their location on disk
short imap[NUM_INODE_BLOCKS];

/*
 * Generates an inode struct with the params given. 
 * DOES NOT MODIFY INODE_FREE_LIST
 */ 
inode_t *create_inode(int file_size, short id, short parent_id, short* direct,
    short num_direct, short single_ind_block, short double_ind_block) {

    inode_t *inode = (inode_t *) malloc(sizeof(inode_t));
    if(!inode) {
        fprintf(stderr, "inode creation failed due to malloc fail\n");
        return NULL;
    }
    inode->file_size = file_size;
    inode->id = id;
    inode->parent_id = parent_id;
    int i;
    if(num_direct > 10) {
        num_direct = 10;
    }
    for(i = 0; i < num_direct; i++) {
        inode->direct[i] = direct[i];
    }
    for(i = num_direct; i < 10; i++) {
        inode->direct[i] = INODE_FIELD_NO_DATA;
    }
    inode->single_ind_block = single_ind_block;
    inode->double_ind_block = double_ind_block;
    return inode;
}

/*
 * Generates a new inode id that is not currently in use. 
 * DOES NOT CHANGE INODE_FREE_LIST
 */ 
short generate_inode_id(bool is_dir) {
    // TODO
    // Find appropriate inode number
    // Add the dir flag if needed (just use a mask)
}

/*
 * Computes the imap key for an inode given its id
 */ 
unsigned char get_block_key_from_id(short inode_id) {
    // Blockid is stored in the less significant byte of the inode id
    return inode_id % 256;
}

/*
 * Computes whether a given inode_id has the dir flag set.
 */ 
bool is_dir(short inode_id) {
    // Flag is the most significant bit of the more significant byte.
    // A masking approach would need us to know the endian-ness of the OS
    // So we're going to go with good old arithmetic
    short mask = 0x1000;
    return mask & inode_id;

    // return inode_id/4096;
}


/*=================================== LLFS API ===============================*/
/*
 * Wipes and formats a fresh virtual disk. Will do so in the current working 
 * directory if alt_disk is omitted.
 * 
 * This formatting entails the following:
 * 1. Clears the vdisk file
 * 2. Writes the magic number (to identify formatting as LLFS), number of blocks 
 *    on the disk, and the number of inodes which can be allowed to exist to the
 *    superblock
 * 3. Sets up bitmaps for the freelists for inodes and disk blocks
 * 4. Sets up the inode map
 * 5. Creates a root directory
 */ 
void initLLFS(FILE* alt_disk) {
    // Clear disk
    void *zeros = calloc(1, BYTES_PER_BLOCK);
    for(int i = 0; i < BLOCKS_ON_DISK; i++){
        vdisk_write(i, zeros, 0, BYTES_PER_BLOCK, alt_disk);
    }

    // Write superblock content
    int sb_content[3] = { MAGIC_NUMBER, BLOCKS_ON_DISK, NUM_INODES };
    vdisk_write(0, sb_content, 0, sizeof(int) * 3, alt_disk);
    
    // Init free lists
    free_block_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    free_inode_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    if(!free_block_list || !free_inode_list) {
        fprintf(stderr, "INIT FAILED: Failed to malloc bitvectors.\n");
        exit(1);
    }
    free_block_list->n = 0;    
    free_inode_list->n = 0;
    
    int i;
    int chars_per_bitvector = BITS_PER_BIT_VECTOR/8;
    // Mark all blocks and inodes as free
    for(i = 0; i < chars_per_bitvector; i++) {
        set_vector_bit(free_block_list, i);        
        set_vector_bit(free_block_list, i);
    }
    // Mark 1st 10 blocks as reserved
    for(i = 0; i < RESERVED_BLOCKS; i++) {
        clear_vector_bit(free_block_list, i);
    }
    free_block_list->next_available = RESERVED_BLOCKS;

    // Init inode map array and mark appropriate blocks as used
    for(i = 0; i < NUM_INODE_BLOCKS; i++) {
        clear_vector_bit(free_block_list, i+RESERVED_BLOCKS);
        imap[i] = i + RESERVED_BLOCKS;
        free_block_list->next_available++;
    }

    // TODO Set up root dir
        // Need an inode for it (remember to set the directory bit)
            // Update next_available in free_inode_list
        // Need a data block for it (mark as used and write to disk)
    
    // inode_t *root_inode = create_inode(0, )
    
    // Write free lists and inode map to disk
    vdisk_write(1, free_block_list->vector, 0, BITS_PER_BIT_VECTOR, alt_disk);
    vdisk_write(2, free_inode_list->vector, 0, BITS_PER_BIT_VECTOR, alt_disk);
    vdisk_write(3, imap, 0,  NUM_INODE_BLOCKS * sizeof(short), alt_disk);
}

// TODO Read and write functionality

// TODO create segment buffer to be sent to disk when full 
// Need a way to ensure this is written on process end
    // One way to do this is to put the onus on the user to perform a manual
    // checkpoint before ending a program which uses this library 
    // (essentially like closing a file system) 

// In addition to writing out the segment, we will want to write updates made to
// free lista and inode map since last checkpoint
