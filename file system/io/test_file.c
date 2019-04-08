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

#define NUM_TESTS 10

// ====  ==== ==== ====  Test method declarations go here ==== ==== ====

// Top Level
bool test_free_list_api();
bool test_create_inode();
bool test_is_dir();
bool test_get_block_key_from_id();
bool test_generate_inode_id();
bool test_get_offset_from_inode_id();
bool test_get_inode_free_list_key();
bool test_init_LLFS();
bool test_get_inode_block();
bool test_create_file_in_root_dir();

// Helper
bool test_superblock_write();
bool test_init_free_lists();
bool test_imap_init();
bool check_inodes_equal(inode_t inode_1, inode_t inode_2);

char *test_names[NUM_TESTS] = {
    "test_free_list_api",
    "test_create_inode",
    "test_is_dir",
    "test_get_block_key_from_id",
    "test_generate_inode_id",
    "test_get_offset_from_inode_id",
    "test_get_inode_free_list_key",
    "test_init_LLFS",
    "test_get_inode_block",
    "test_create_file_in_root_dir"
};

// ==== ==== ==== ==== Helper methods go here ==== ==== ==== ==== ==== 


int main(int argc, char **argv) {
    printf("\nRunning tests for LLFS\n");
    
    bool (*tests[NUM_TESTS]) (); // ADD TESTS HERE
    tests[0] = test_free_list_api;
    tests[1] = test_create_inode;
    tests[2] = test_is_dir;
    tests[3] = test_get_block_key_from_id;
    tests[4] = test_generate_inode_id;
    tests[5] = test_get_offset_from_inode_id;
    tests[6] = test_get_inode_free_list_key;
    tests[7] = test_init_LLFS;
    tests[8] = test_get_inode_block;
    tests[9] = test_create_file_in_root_dir;

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

// ==== ==== ==== ==== Tests go here ==== ==== ==== ==== ==== 
bool test_free_list_api() {
    short blocks_to_test[4] = {0,1,1000,4095};
    bitvector_t *vect = (bitvector_t *) malloc(sizeof(bitvector_t));
    vect->n = 0;
    vect->next_available = 0; 
    // unsigned char free_list[BYTES_PER_BLOCK];
    for(int i = 0; i < 4; i++) {
        clear_vector_bit(vect, blocks_to_test[i]);
        if(test_vector_bit(vect, blocks_to_test[i]) || vect->n != 1) {
            return false;
        }
        set_vector_bit(vect, blocks_to_test[i]);
        if(!test_vector_bit(vect, blocks_to_test[i]) || vect->n != 0) {
            return false;
        }
        clear_vector_bit(vect, blocks_to_test[i]);
        if(test_vector_bit(vect, blocks_to_test[i]) || vect->n != 1) {
            return false;
        }
        set_vector_bit(vect, blocks_to_test[i]);
        if(!test_vector_bit(vect, blocks_to_test[i]) || vect->n != 0) {
            return false;
        }
    }
    return true;
}

bool test_create_inode() {
    int file_size = 100100;
    short direct[9] = {1,2,3,4,5,6,7,8,9};
    short indirect1 = 11;
    short indirect2 = 12;
    short id = 100;
    short parent_id = 101;
    inode_t *inode = create_inode(file_size, id, parent_id, direct, 9, indirect1, indirect2);

    if(inode->file_size != file_size) {
        return false;
    }
    for(int i = 0; i < 9; i++) {
        if(inode->direct[i] != direct[i]) {
            fprintf(stderr, "\tvvv full direct blocks don't match\n");
            return false;
        }
    }
    if(inode->direct[9] != INODE_FIELD_NO_DATA) {
        fprintf(stderr, "\tvvv empty direct blocks bad value\n");
        return false;
    }
    if(inode->single_ind_block != indirect1) {
        fprintf(stderr, "\tvvv single indirect block no match\n");
        return false;
    }
    if(inode->double_ind_block != indirect2) {
        fprintf(stderr, "\tvvv double indirect block no match\n");
        return false;
    }
    if(inode->id != id) {
        fprintf(stderr, "\tvvv id doesnt match\n");
        return false;
    }
    if(inode->parent_id != parent_id) {
        fprintf(stderr, "\tvvv parent id doesnt match\n");
        return false;
    }
    return true;
}

bool test_is_dir() {
    short non_dir = 0x8FFF;
    short dir = 0x1000;
    if(!is_dir(dir)) {
        return false;
    }
    if(is_dir(non_dir)) {
        return false;
    }
    return true;
}

bool test_get_block_key_from_id() {
    // Block key is "middle" 8 bits (4 from each byte)
    short id = 0x1234;
    if(get_block_key_from_id(id) != 0X23) {
        return false;
    }
    return true;
}

bool test_get_offset_from_inode_id() {
    // Offset is 4 least significant bytes
    short x = 0xABCD;
    short y = 0x1234; 
    if(get_offset_from_inode_id(x) != 0x0D) {
        return false;
    }
    if(get_offset_from_inode_id(y) != 0x04) {
        return false;
    }
    return true;
}

/*
 * Relies on functions is_dir, get_block_key_from_id, get_offset_from_inode_id
 */ 
bool test_generate_inode_id() {
    // Needs the inode free list to be initialized for it to work
    bitvector_t *free_inode_list = _init_free_inode_list();
    
    // Test with valid next_available
    free_inode_list->next_available = 10;
    short id = generate_inode_id(false);
    if(is_dir(id)) {
        return false;
    }
    if(get_block_key_from_id(id) != 0) {
        return false;
    }
    if(get_offset_from_inode_id(id) != 10) {
        return false;
    }

    // Test with invalid next_available
    clear_vector_bit(free_inode_list, 10);
    id = generate_inode_id(true);
    if(!is_dir(id)) {
        return false;
    }
    if(get_block_key_from_id(id) != 0) {
        return false;
    }
    if(get_offset_from_inode_id(id) != 0) {
        return false;
    }
    return true;
}

bool test_get_inode_free_list_key() {
    // TODO
    short id1 = 0x1123; // key should be 0x123
    short id2 = 0x1000; // key should be 0
    short id3 = 0x0FFF; // key should be 0xFFF
    if(get_inode_free_list_key(id1) != 0x123) {
        return false;
    }
    if(get_inode_free_list_key(id2) != 0) {
        return false;
    }
    if(get_inode_free_list_key(id3) != 0xFFF) {
        return false;
    }
    return true;
}

/*
 * The init function does a whole load of stuff, so this test is going to be 
 * pretty involved. 
 * There are a handful of actions that init is responsible for, so we'll make 
 * this test accordingly modular.
 * Those actions are:
 *      1. Writes the superblock on disk
 *      2. Sets up both bitmaps
 *      3. Sets up the inode map
 *      4. Creates a root directory
 */ 
bool test_init_LLFS() {
    VERBOSE_PRINT("\n"); // Prettify output when combined with LLFS output
    init_LLFS(NULL);
    if(!test_superblock_write()) {
        printf("  Failed test_superblock_write  ");
        return false;
    }
    if(!test_init_free_lists()) {
        printf("  Failed test_init_free_lists  ");
        return false;
    }
    if(!test_imap_init()) {
        printf("  Failed test_imap_init  ");
        return false;
    }
    VERBOSE_PRINT("\t\t"); // Formatting
    return true;
}

/*
 * Helper for test_init_LLFS
 */ 
bool test_superblock_write() {
    // Read block 0 of the disk and verify that the 3 entries are correct
    int content[BYTES_PER_BLOCK/sizeof(int)];
    int oracle[3] = {MAGIC_NUMBER, BLOCKS_ON_DISK, NUM_INODES};
    vdisk_read(0, content, NULL);
    // printf("MAGIC_NUMBER: %d\n", content[0]);
    // printf("BLOCKS_ON_DISK: %d\n", content[1]);
    // printf("NUM_INODES: %d\n", content[2]);
    for(short i = 0; i < 3; i++) {
        if(content[i] != oracle[i]) {
            return false;
        }
    }
    return true;
}

/*
 * Helper for test_init_LLFS
 * Relies on bitvector API, tested in test_free_list_api()
 */ 
bool test_init_free_lists() {
    bitvector_t *free_block_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    bitvector_t *free_inode_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    vdisk_read(1, free_block_list->vector, NULL);
    vdisk_read(2, free_inode_list->vector, NULL);
    short i;

    // only reserved blocks and imap blocks should be marked as used
    int total_blocks_used = RESERVED_BLOCKS + NUM_INODE_BLOCKS;
    for(i = 0; i < total_blocks_used; i++) {
        if(test_vector_bit(free_block_list, i)) {
            printf("Block %d should not be available\n", i);
            return false;
        }
    } 
    for(i = total_blocks_used; i < BLOCKS_ON_DISK; i++) {
        if(!test_vector_bit(free_block_list, i)) {
            printf("Block %d should be available\n", i);
            return false;
        }
    }

    // only the root inode should be marked as used
    if(test_vector_bit(free_inode_list, 0)) {
        printf("Inode 0 should not be available\n");
        return false;
    }
    for(i = 1; i < NUM_INODES; i++) {
        if(!test_vector_bit(free_inode_list, i)) {
            printf("Inode %d should be available\n", i);
            return false;
        }
    }
    return true;
}

/*
 * Helper for test_init_LLFS
 */ 
bool test_imap_init() {
    // Imap is stored in block 3. There are 256 shorts as entries.
    short imap[NUM_INODE_BLOCKS];
    vdisk_read(3, imap, NULL);
    short i;

    // Each entry should point to a consecutive block to start with.
    for(i = 0; i < NUM_INODE_BLOCKS; i++) {
        if(imap[i] != RESERVED_BLOCKS + i) {
            return false;
        }
    }
    
    // Retrieve and verify the root's inode.
    inode_t inode_block[INODES_PER_BLOCK];
    vdisk_read(imap[0], inode_block, NULL);
    inode_t* root_inode = inode_block;
    if(ROOT_ID != root_inode->id) {
        return false;
    }
    if(0 != root_inode->file_size) {
        return false;
    }
    if(ROOT_ID != root_inode->parent_id) {
        return false;
    }
    if(INODE_FIELD_NO_DATA != root_inode->direct[0]) {
        return false;
    }
    if(INODE_FIELD_NO_DATA != root_inode->single_ind_block) {
        return false;
    }
    if(INODE_FIELD_NO_DATA != root_inode->single_ind_block) {
        return false;
    }
    return true;
}

/*
 * Helper created for test_get_inode_block
 */ 
bool check_inodes_equal(inode_t inode_1, inode_t inode_2) {
    if(inode_1.id != inode_2.id) {
        VERBOSE_PRINT("id mismatch")
        return false;
    }
    if(inode_1.parent_id != inode_2.parent_id) {
        VERBOSE_PRINT("parent id mismatch")
        return false;
    }
    if(inode_1.file_size != inode_2.file_size) {
        VERBOSE_PRINT("file size mismatch")
        return false;
    }
    if(inode_1.single_ind_block != inode_2.single_ind_block) {
        VERBOSE_PRINT("single indirect block mismatch")
        return false;
    }
    if(inode_1.double_ind_block != inode_2.double_ind_block) {
        VERBOSE_PRINT("double indirect block mismatch")
        return false;
    }
    int i;
    for(i = 0; i < 10; i++) {
        if(inode_1.direct[i] != inode_2.direct[i]) {
            VERBOSE_PRINT("direct block #%d mismatch", i);
            return false;
        }
    }
    return true;
}

/*
 * Relies on create_inode() and get_block_key_from_id()
 */ 
bool test_get_inode_block() {
    inode_t oracle_inodes[16];
    inode_t *result_inodes;
    int i;

    short direct1[10] = {1,2,3,4,5,7,8,9,1000};
    short direct2[5] = {-1,-2,-3,-4,-5};

    oracle_inodes[0] = *create_empty_inode(0x0000, ROOT_ID);
    oracle_inodes[1] = *create_empty_inode(0x0001, ROOT_ID);
    for(i = 2; i < 16; i+=2) {
        oracle_inodes[i] = *create_inode(i, i, ROOT_ID, direct1, 10, i/2, 2*i);
        oracle_inodes[i + 1] = *create_inode(i+1, i+1, ROOT_ID, direct2, 5, (i+1)/2, 2*i + 2);
    }

    vdisk_write(10, oracle_inodes, 0, sizeof(inode_t) * 16, NULL);

    result_inodes = get_inode_block(get_block_key_from_id(0x0000));

    for(i = 0; i < 16; i++) {
        if(!check_inodes_equal(oracle_inodes[i], result_inodes[i])) {
            printf("\nfailed for inode #%d\n\t", i);
            return false;
        }
    }
    return true;
}

bool test_create_file_in_root_dir() {
    // TODO
    printf("\tUNIMPLEMENTED\t");
    return false;
}