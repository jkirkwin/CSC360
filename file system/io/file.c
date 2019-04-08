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


/*
 * Marks the next available block of memory as used and returns its number.
 * 
 * Does not update next available block to be the next block.
 * 
 * Does not call defrag if this fills the disk; it is up to client code to do so
 * 
 * @pre the bitvector must be set up correctly (i.e. next_available must be 
 *      correct)
 * @pre there must be a valid data block to use; i.e. if defragging is necessary
 *      it must be done before calling this function.
 */ 
short consume_block() {
    short na = free_block_list->next_available;
    if(!test_vector_bit(free_block_list, na)) {
        fprintf(stderr, "WARNING: Unsafe consume_block() call\n");
        return -1;
    }
    clear_vector_bit(free_block_list, na);        
    return na;
}

/*============================ INODES AND IMAP ==============================*/

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
 * A wrapper for create_inode in the case that the inode will not yet have any
 *  associated data blocks.
 */ 
inode_t *create_empty_inode(short id, short parent_id) {
    return create_inode(0, id, parent_id, NULL, 0, INODE_FIELD_NO_DATA, INODE_FIELD_NO_DATA);
}

/*
 * Generates a new inode id that is not currently in use.
 * Uses the next_available field if unused, otherwise scans for the lowest 
 * unused inode number and uses that. Sets next_available to this value if this
 * happens. Unused bits are cleared.
 * DOES NOT CHANGE INODE_FREE_LIST.
 */ 
short generate_inode_id(bool dir_flag) {
    // Find appropriate inode number
    if(free_inode_list->n >= NUM_INODES) {
        fprintf(stderr, "WARNING: cannot generate inode id - all nodes allocated\n");
        return -1;
    }
    short id;
    if(test_vector_bit(free_inode_list, free_inode_list->next_available)){
        // Use what was previously marked as next_available
        id = free_inode_list->next_available;
    } else {
        // Find the lowest available inode id, use it, and set it as next_avaliable
        short i;
        for(i = 0; i < BITS_PER_BIT_VECTOR; i++) {
            if(test_vector_bit(free_inode_list, i)) {
                free_inode_list->next_available = i;
                id = i;
                break;
            }
        }
    }

    id = id & 0x7F; // Clear unused bits
    if(dir_flag) {
        id = id | 0x1000; // Set dir flag
    }
    return id;
}

/*
 * Computes the imap key for an inode given its id
 */ 
unsigned char get_block_key_from_id(short inode_id) {
    // Blockid is stored in the less significant byte of the inode id
    return (inode_id >> 4) & 0xFF;
}

/*
 * Computes the offset within the inode bock for an inode given its id
 */ 
unsigned char get_offset_from_inode_id(short inode_id) {
    return inode_id & 0x0F;
}

/*
 * Computes whether a given inode_id has the dir flag set.
 * Requires a big-endian architecture (which the ssh server is)
 */ 
bool is_dir(short inode_id) {
    // Flag is the most significant bit of the more significant byte.
    short mask = 0x1000;
    return mask & inode_id;
}

/*
 * Gives the key used in the inode free list for an inode given its id
 */ 
short get_inode_free_list_key(short inode_id) {
    return inode_id & 0x0FFF;
}

/*
 * Returns the content of the specified inode block as an array of 16 inodes.
 */ 
inode_t* get_inode_block(short inode_block_key) {
    inode_t *inodes = (inode_t *) malloc(BYTES_PER_BLOCK);
    if(!inodes) {
        fprintf(stderr, "ERROR: Could not allocate memory for inode block\n");
        return NULL;
    }
    if(!vdisk_read(imap[inode_block_key], inodes, NULL)) {
        fprintf(stderr, "ERROR: Could not read inode block\n");
        free(inodes);
        return NULL;
    }
    return inodes;
} 

/*=================================== LLFS API ===============================*/

/*
 * Wipes and formats a fresh virtual disk in the current working directory.
 * 
 * This formatting entails the following:
 * 1. Writes the magic number (to identify formatting as LLFS), number of blocks 
 *    on the disk, and the number of inodes which can be allowed to exist to the
 *    superblock
 * 2. Sets up bitmaps for the freelists for inodes and disk blocks
 * 3. Sets up the inode map
 * 4. Creates a root directory
 */ 
void init_LLFS() {
    VERBOSE_PRINT("Formatting Disk\n");

    // Write superblock content
    int sb_content[3] = { MAGIC_NUMBER, BLOCKS_ON_DISK, NUM_INODES };
    vdisk_write(0, sb_content, 0, sizeof(int) * 3, NULL);
    
    // Init free lists
    free_block_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    free_inode_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    if(!free_block_list || !free_inode_list) {
        fprintf(stderr, "INIT FAILED: Failed to malloc bitvectors.\n");
        exit(1);
    }
    free_block_list->n = 0;    
    free_inode_list->n = 0;
    free_block_list->next_available = 0;

    int i;
    // Mark all blocks and inodes as free
    for(i = 0; i < BITS_PER_BIT_VECTOR; i++) {
        set_vector_bit(free_inode_list, i);        
        set_vector_bit(free_block_list, i);
    }

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

    // Set up root dir (its its own parent, as per linux convention) and write to disk immediately
    free_inode_list->next_available = 1; 
    clear_vector_bit(free_inode_list, get_inode_free_list_key(ROOT_ID));
    inode_t *root_inode = create_inode(0, ROOT_ID, ROOT_ID, NULL, 0, 
            INODE_FIELD_NO_DATA, INODE_FIELD_NO_DATA);
    vdisk_write(get_block_key_from_id(ROOT_ID) + RESERVED_BLOCKS, root_inode, 
            get_offset_from_inode_id(ROOT_ID), sizeof(inode_t), NULL);

    // Write free lists and inode map to disk
    vdisk_write(1, free_block_list->vector, 0, BITS_PER_BIT_VECTOR/8, NULL);
    vdisk_write(2, free_inode_list->vector, 0, BITS_PER_BIT_VECTOR/8, NULL);
    vdisk_write(3, imap, 0,  NUM_INODE_BLOCKS * sizeof(short), NULL);

    VERBOSE_PRINT("Formatting done. Root directory created.\n");
}


/*
 * Flush all cached changes to disk. Ensures that disk is consistent with state
 * in memory.
 * 
 * Empties the checkpoint buffer.
 * 
 * Defrags the memory if necessary to complete the flush.
 */ 
void flush_LLFS() {
    // TODO
}

/* 
 * Consolidate the file system, filling in empty chunks of the disk to make more
 * room at the end.
 * 
 * Updates the free lists and imap appropriately. Changes are made persistent on
 * completion.
 * 
 * The new checkpoint region (the end of the current log) will be pointed to by
 * free_block_list->next_available.
 */ 
void defrag_LLFS() {
    // TODO
}

/*
 * Flushes changes to the disk and performs teardown tasks.
 */ 
void terminate_LLFS() {
    flush_LLFS();
    // TODO
    // Free memory and close any file pointers

}

/*
 * Creates a file with the name given in the parent directory specified.
 * 
 * Returns NULL if unsuccessful. Returns a pointer to the inode for the new file
 * otherwise.
 */ 
inode_t *create_file(char *filename, char *path_to_parent_dir) {
    // TODO
    // For now we'll make the following simplifications:
        // 1. the parent will be assumed to be root.
        // 2. changes will be written directly to disk (without using a checkpoint 
        //    buffer)

    // note that in creating a file, we only ever need to consume one block
    // as all we're really doing is changing the inode block.
    // TODO add following checks
    // if(checkpoint buffer is full) {
    //     fflush();
    // }


    // Use root dir as default parent for now.
    int root_inode_block = imap[get_block_key_from_id(ROOT_ID)];
    inode_t *root_inode = get_inode_block(root_inode_block);

    // TODO Find parent directory instead of using root as is done above.
    // inode_t *parent_inode = find_dir(path_to_parent_dir);
    // if(!parent_inode) {
    //     fprintf(stderr, "No such directory exists. Cannot create file.\n");
    //     return NULL;
    // }
    inode_t *parent_inode = root_inode; // TODO change

    // TODO Validate filename 
    // - Ensure it is within the allowed bounds
    // - Ensure it does not contain any slashes or other special characters other 
    //   than a single '.'
    // - Check that a file with that name does not already exist in the parent
    // - directory 

    // Allocate inode for the file
    short inode_id = generate_inode_id(false);
    inode_t *file_inode = create_empty_inode(inode_id, parent_inode->id);
    if(!file_inode) {
        fprintf(stderr, "Could not allocate inode. Cannot create file.\n");
        return NULL;
    }
    clear_vector_bit(free_inode_list, get_inode_free_list_key(inode_id));

    // TODO Add the inode block to the checkpoint buffer

    // mark the previous data block as unused.
    set_vector_bit(free_block_list, imap[get_block_key_from_id(inode_id)]);
    
    // TODO check whether we need to defrag before consuming the block

    // get a new spot for the inode block and update the inode map
    short new_inode_location = consume_block();
    free_block_list->next_available++; // TODO check whether we need to defrag instead of this
    imap[get_block_key_from_id(inode_id)] = new_inode_location; 
        
    VERBOSE_PRINT("Created new file '%s' in root directory\n", filename);
    return file_inode;
}

/*
 * Creates a subdirectory of the given parent directory with the specified name.
 * 
 * Returns NULL if successful. Returns a pointer to the indode for the new 
 * directory otherwise.
 */ 
inode_t *mkdir(char * dirname, char *path_to_parent_dir) {
    // TODO
    return NULL;
}

/*
 * Writes the content given to the file specified. Will overwrite existing 
 * content if the offset is less than the length of the file. Content_length is 
 * in bytes.
 * 
 * Returns the number of bytes written. -1 on error.
 * 
 * Fails if the file name given does not correspond to a non-directory file in 
 * the file system.
 * 
 * Fails if offset is negative or is greater than the length of the file.  
 */ 
int write(void* content, int content_length, int offset, char *filename) {
    // TODO
    return -1;
}

/*
 * Appends the content given to the end of the specified file. Not content will 
 * be overwritten. Content_length is in bytes.
 * 
 * Returns the number of bytes written. -1 on error.
 * 
 * Fails if the file name given does not correspond to a non-directory file in 
 * the file system.
 */ 
int append(char *content, int content_length, char *filename) {
    // TODO 
    return -1;
}

/*
 * Returns a NULL-terminated list of the filenames of all files in the 
 * directory. Directories are indicated with a trailing '/'.
 */ 
char **get_dir_contents(char *dirname) {
    // TODO
    return NULL;
}

/*
 * Reads up to buffer_size bytes from the specified file into the buffer. Does
 * not pad the buffer if it exceeds the size of the file.
 * 
 * Returns the number of bytes read into the buffer, -1 on error.
 * 
 * Fails if the file name given does not correspond to a non-directory file in 
 * the file system.
 */ 
int read_file(void *buffer, int buffer_size, char *filename) {
    // TODO
    return -1;
}

/*
 * Deletes the specified file. If the file is a directory, it must be empty in
 * order to be deleted.
 * 
 * The root directory cannot be removed.
 * 
 * Returns true iff the deletion was successful.
 */ 
bool rm(char *filename) {
    // TODO
    return false;
}

/*
 * Returns the inode of the specified directory. Returns NULL if no such 
 * directory exists.
 */ 
inode_t* find_dir(char* dirpath) {
    // TODO
    return NULL;
}

// TODO create segment buffer to be sent to disk when full 
// Need a way to ensure this is written on process end
    // One way to do this is to put the onus on the user to perform a manual
    // checkpoint before ending a program which uses this library 
    // (essentially like closing a file system) 

// In addition to writing out the segment, we will want to write updates made to
// free list and inode map since last checkpoint



/*============================= Testing helpers =============================*/

/*
 * To allow unit tests independant of initLLFS().
 * Sets free_inode_list to have all entries = 1 and n, next_available to 0.
 * Returns a pointer to the list.
 */ 
bitvector_t* _init_free_inode_list() {
    free_inode_list = (bitvector_t *) malloc(sizeof(bitvector_t));
    int i;
    for(i = 0; i < BITS_PER_BIT_VECTOR; i++) {
        set_vector_bit(free_inode_list, i);
    }
    free_inode_list->n = 0;
    free_inode_list->next_available = 10;
    return free_inode_list;
}

void print_inode_details(inode_t* inode) {
    printf("Inode Details:\n");
    short id = inode->id;
    printf("\t hex id: %x\n", id&0xFFFF);
    printf("\t decimal id: %d\n", id);
    printf("\t free list key: %d\n", get_inode_free_list_key(id));
    printf("\t block key: %d\n", get_block_key_from_id(id));
    printf("\t offset: %d\n", get_offset_from_inode_id(id));
    printf("\t is_dir: %s\n", is_dir(id) ? "true" : "false");
    printf("\t parent id: %d\n", inode->parent_id);
    printf("\t file size: %d\n", inode->file_size);
    printf("\t single ind hex: %x\n", inode->single_ind_block & 0xFFFF);
    printf("\t double ind hex: %x\n", inode->double_ind_block & 0xFFFF);
}