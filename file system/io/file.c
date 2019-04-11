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

/* ============================== Globals ============================== */
// bit = 1 if available, 0 if in use
bitvector_t *free_block_list;
bitvector_t *free_inode_list;

checkpoint_buffer_t *checkpoint_buffer;

/*========================= Bit Vectors ===================================*/
// TODO Ensure changes go to disk eventually
// TODO Ensure that this is loaded correctly

// Reference: http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html


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
 * Marks every item as used.
 */ 
void clear_entire_vector(bitvector_t *vector) {
    int i;
    unsigned char* arr = vector->vector;
    for(i = 0; i < BITS_PER_BIT_VECTOR/8; i++) {
        arr[i] = 0;
    }
    vector->n = BITS_PER_BIT_VECTOR;
}

/*
 * Marks every item as unused. Sets next_available to 0;
 */ 
void set_entire_vector(bitvector_t *vector) {
    int i;
    unsigned char* arr = vector->vector;
    for(i = 0; i < BITS_PER_BIT_VECTOR/8; i++) {
        arr[i] = 0xff;
    }
    vector->n = 0;
    vector->next_available = 0;
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
 * Retrieves the latest copy of the block from disk or the checkpoint buffer
 * as appropriate.
 */ 
inode_t* get_inode_block(short inode_block_key) {
    inode_t *inodes = (inode_t *) malloc(BYTES_PER_BLOCK);
    if(!inodes) {
        fprintf(stderr, "ERROR: Could not allocate memory for inode block\n");
        return NULL;
    }
    short block_num = imap[inode_block_key];
    int end_of_disk_log = free_block_list->next_available - 1 - checkpoint_buffer->blocks_used;
    if(block_num <= end_of_disk_log) {
        if(!vdisk_read(block_num, inodes, NULL)) {
            fprintf(stderr, "ERROR: Could not read inode block\n");
            free(inodes);
            return NULL;
        }
    } else {
        // retrieve it from the buffer
        short offset = block_num - free_block_list->next_available - 1;
        return (inode_t *) checkpoint_buffer->buffer[offset]->content;
    }
    return inodes;
} 

/*=================================== LLFS API ===============================*/


/*
 * Adds a block of content to the checkpoint buffer and marks the associated
 * inode as dirty.
 * 
 * Does NOT increment free_block_list->next_available
 * 
 * @pre there must be room for at least one more block in the checkpoint buffer 
 * 
 * @pre init_checkpoint_buffer must have been called
 * 
 * returns true iff successful
 */ 
bool add_entry_to_checkpoint_buffer(void* content, int content_length, int inode_id) {
    int length = content_length;
    if(length > BYTES_PER_BLOCK) {
        length = BYTES_PER_BLOCK;
        fprintf(stderr, "WARNING: truncating content passed to checkpoint buffer\n");
    }
    cb_entry_t *new_entry = calloc(1, sizeof(cb_entry_t));
    if(!new_entry) {
        fprintf(stderr, "ERROR: Could not allocate memory for new cb entry\n");
        return false;
    }
    new_entry->content = malloc(length);
    if(!new_entry->content) {
        fprintf(stderr, "ERROR: Could not allocate memory for cb entry contents\n");
        return false;
    }

    memcpy(new_entry->content, content, length);
    new_entry->content_length = length;
    
    int offset = checkpoint_buffer->blocks_used;
    checkpoint_buffer->buffer[offset] = new_entry; // TODO Make sure this gets free'd
    checkpoint_buffer->blocks_used++;
    
    clear_vector_bit(checkpoint_buffer->dirty_inode_list, get_inode_free_list_key(inode_id));

    return true;
}

/*
 * Allocates memory for the checkpoint buffer and sets up the structure. Also 
 * returns a pointer to it.
 */ 
checkpoint_buffer_t* init_checkpoint_buffer() {
    checkpoint_buffer_t *cb = malloc(sizeof(checkpoint_buffer_t));
    if(!cb) {
        fprintf(stderr, "ERROR: failed to allocate memory for checkpoint buffer struct.\n");
        exit(1);
    }
    cb->blocks_used = 0;
    cb->dirty_inode_list = malloc(sizeof(bitvector_t));
    if(!cb->dirty_inode_list) {
        fprintf(stderr, "ERROR: failed to allocate memory for checkpoint buffer inode list.\n");
        exit(1);
    }
    set_entire_vector(cb->dirty_inode_list);
    checkpoint_buffer = cb;
    return cb;
}

/*
 * Free memory used for checkpoint buffer and set global pointer to NULL
 */ 
void destroy_checkpoint_buffer() {
    checkpoint_buffer_t *cb = checkpoint_buffer;
    checkpoint_buffer = NULL;
    free(cb->dirty_inode_list);
    free(cb);
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
 
    init_checkpoint_buffer();

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
    vdisk_write(FREE_BLOCK_LIST_BLOCK_NUMBER, free_block_list->vector, 0, BITS_PER_BIT_VECTOR/8, NULL);
    vdisk_write(FREE_INODE_LIST_BLOCK_NUMBER, free_inode_list->vector, 0, BITS_PER_BIT_VECTOR/8, NULL);
    vdisk_write(IMAP_BLOCK_NUMBER, imap, 0,  NUM_INODE_BLOCKS * sizeof(short), NULL);

    VERBOSE_PRINT("Formatting done. Root directory created.\n");
}


/*
 * Flush all cached changes to disk. Ensures that disk is consistent with state
 * in memory.
 * 
 * Empties the checkpoint buffer.
 * 
 * Defrags the memory if necessary to complete the flush.
 * 
 */ 
void flush_LLFS() {
    VERBOSE_PRINT("Flushing checkpoint buffer\n");

    // Defrag if disk is full
    short block_number = free_block_list->next_available;
    if(checkpoint_buffer->blocks_used >= BLOCKS_ON_DISK - block_number) {
        defrag_LLFS(); // TODO this is unimplemented
    }

    // Empty the checkpoint buffer after writing its contents to disk
    cb_entry_t **buffer = checkpoint_buffer->buffer;
    int i, content_length;
    bool result;
    void *content;
    for(i = 0; i < checkpoint_buffer->blocks_used; i++) {
        content = buffer[i]->content;
        content_length = buffer[i]->content_length;
        result = vdisk_write(block_number++, content, 0, content_length, NULL);
        if(!result) {
            fprintf(stderr, "ERROR: Flush Failed. Could not write buffer to disk. Terminating flush.\n");
            // reset checkpoint buffer 
            destroy_checkpoint_buffer();
            init_checkpoint_buffer();
            return;
        }
    }
    
    free_block_list->next_available = block_number;
    for(i = 0; i < checkpoint_buffer->blocks_used; i++) {
        free(checkpoint_buffer->buffer[i]);
    }
    checkpoint_buffer->blocks_used = 0;
    set_entire_vector(checkpoint_buffer->dirty_inode_list);

    // Update the data structures on disk
    result = vdisk_write(FREE_BLOCK_LIST_BLOCK_NUMBER, free_block_list->vector,
        0, BITS_PER_BIT_VECTOR/8, NULL);
    if(!result) {
        fprintf(stderr, "ERROR: Flush Failed. Terminating flush.\n");
        exit(1);
    }
    result = vdisk_write(FREE_INODE_LIST_BLOCK_NUMBER, free_inode_list->vector,
        0, BITS_PER_BIT_VECTOR/8, NULL);
    if(!result) {
        fprintf(stderr, "ERROR: Flush Failed. Terminating flush.\n");
        exit(1);
    }
    result = vdisk_write(IMAP_BLOCK_NUMBER, imap,
        0, BYTES_PER_BLOCK, NULL);
    if(!result) {
        fprintf(stderr, "ERROR: Flush Failed. Terminating flush.\n");
        exit(1);
    }
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
    destroy_checkpoint_buffer();
    // TODO Free memory and close any file pointers

}


/*
 * Returns a copy of the content of the specified block. If the block number
 * is for a block which has not yet been flushed, we return the content from the 
 * checkpoint buffer. Otherwise we must read from the disk.
 */ 
void *get_block(short block_number) {
    VERBOSE_PRINT("Retrieving block %d\n", block_number);
    char * buffer = malloc(BYTES_PER_BLOCK);
    if(!buffer) {
        fprintf(stderr, "ERROR: Failed to allocated space for buffer\n");
        exit(1);
    }
    int disk_log_end = free_block_list->next_available - checkpoint_buffer->blocks_used - 1; // the last block synched to disk
    if(block_number <= disk_log_end) {
        VERBOSE_PRINT("Reading the block from disk\n");
        vdisk_read(block_number, buffer, NULL);
    } else {
        int offset = block_number - disk_log_end - 1;
        VERBOSE_PRINT("Reading the block from the checkpoint buffer [offset=%d]\n", offset);
        void *content = checkpoint_buffer->buffer[offset]->content;
        int content_length = checkpoint_buffer->buffer[offset]->content_length;
        memcpy(buffer, content, content_length);
    }
    return buffer;
}

/* 
 * Checks that the given filename is valid. File names are not allowed to have 
 * any special characters in them. Filenames must be null-terminated and have a
 * size limit.
 */ 
bool is_valid_filename(char *filename) {
    int length = strlen(filename);
    if(length > MAX_FILENAME_LENGTH || length <= 0) {
        printf("File name must 1 to 30 characters and be null terminated\n");
        return false;
    }
    for(int i = 0; i < length; i++) {
        char c = filename[i];
        if( !('a' <= c && c <= 'z') && !('A' <= c && c <= 'Z') && !('1' <= c && c <= '9')) {
            printf("special characters cannot exist in file names\n");
            return false;
        }
    }
    return true;
}

/*
 * Creates a file with the name given in the parent directory specified.
 * 
 * Returns NULL if unsuccessful. Returns a pointer to the inode for the new file
 * otherwise.
 */ 
inode_t *create_file(char *filename, char *path_to_parent_dir) {
    // TODO test
    
    // TODO For now the parent will be assumed to be root.

    // Ensure we have room for this.
    if(checkpoint_buffer->blocks_used >= CHECKPOINT_BUFF_SIZE) {
        flush_LLFS();
    }

    // Use root dir as default parent for now.
    int root_inode_block = imap[get_block_key_from_id(ROOT_ID)];
    inode_t *root_inode = get_inode_block(root_inode_block); // TODO free this

    // TODO Find parent directory instead of using root as is done above.
    // inode_t *parent_inode = find_dir(path_to_parent_dir);
    // if(!parent_inode) {
    //     fprintf(stderr, "No such directory exists. Cannot create file.\n");
    //     return NULL;
    // }
    inode_t *parent_inode = root_inode; // TODO change

    // Validate filename form
    if(!is_valid_filename(filename)) {
        return NULL;
    }

    // Check that a file with that name does not already exist in the parent directory
    dir_entry_t** dir_entries = get_dir_entries(parent_inode); // TODO this is unimplemented
    int i = 0, filename_len = strlen(filename);
    dir_entry_t *p = dir_entries[0];
    while(p != NULL) { 
        if(strlen(p->filename) != filename_len || strncmp(filename, p->filename, filename_len)) {
            printf("A file of this name already exists in this directory\n");
            free(dir_entries);
            return NULL;
        }
        p = dir_entries[i++];
    } 
    free(dir_entries);

    // Allocate inode for the file
    short inode_id = generate_inode_id(false);
    inode_t *file_inode = create_empty_inode(inode_id, parent_inode->id);
    if(!file_inode) {
        fprintf(stderr, "Could not allocate inode. Cannot create file.\n");
        return NULL;
    }
    clear_vector_bit(free_inode_list, get_inode_free_list_key(inode_id));

    // Add the inode block to the checkpoint buffer
    short inode_block_key = get_block_key_from_id(inode_id);
    inode_t *inode_block = get_inode_block(inode_block_key); 
    short offset = get_offset_from_inode_id(inode_id);
    inode_block[offset] = *file_inode;
    add_entry_to_checkpoint_buffer(inode_block, sizeof(inode_t) * INODES_PER_BLOCK, inode_id);
    free(inode_block);

    // TODO check whether we need to defrag before consuming the block

    // mark the previous data block as unused and replace with the new one
    set_vector_bit(free_block_list, imap[get_block_key_from_id(inode_id)]);
    short new_inode_location = consume_block();    
    free_block_list->next_available++; 

    // update the inode map
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
 * Return all blocks associated with the inode in the form of a null-terminated 
 * void **.
 */ 
void **get_blocks(inode_t *inode) {
    // TODO Test
    
    VERBOSE_PRINT("Getting blocks for inode %d\n", inode->id);

    // alloc enough memory for pointers plus a null terminator
    int allowed_blocks = 11;
    bool has_single_ind = false;
    if(inode->single_ind_block != INODE_FIELD_NO_DATA) {
        has_single_ind = true;
        allowed_blocks += 256;
    }
    bool has_double_ind = false;
    if(inode->double_ind_block != INODE_FIELD_NO_DATA) {
        has_double_ind = true;
        allowed_blocks += 256*256;
    }
    void **blocks = calloc(allowed_blocks, sizeof(void *)); // Calloc ensures null-termination
    VERBOSE_PRINT("Allocated space for %d block pointers (including null terminator)\n", allowed_blocks);

    // Direct blocks
    int index;
    short block_num;
    for(index = 0; index < 10; index++) {
        block_num = inode->direct[index];
        if(block_num == INODE_FIELD_NO_DATA) {
            blocks[index] = NULL;
            return blocks;
        }
        blocks[index] = get_block(block_num);
    }
    VERBOSE_PRINT("Added Direct Blocks\n");

    // Single indirect blocks
    if(!has_single_ind) {
        return blocks;
    }
    short *ind_block = get_block(inode->single_ind_block);
    for(int i = 0; i < 256; i++) {
        block_num = ind_block[i];
        if(block_num == INODE_FIELD_NO_DATA) {
            blocks[index] = NULL;
            free(ind_block);
            return blocks;
        }
        blocks[index++] = get_block(block_num); // seg faulting
    }
    free(ind_block);
    VERBOSE_PRINT("Added Single Indirect Blocks\n");

    // Double indirect blocks
    if(!has_double_ind) {
        return blocks;
    }
    short *double_ind_block = get_block(inode->double_ind_block);
    for(int i = 0; i < 256; i++) {
        if(double_ind_block[i] == INODE_FIELD_NO_DATA) {
            free(double_ind_block);
            return blocks;
        }
        ind_block = get_block(double_ind_block[i]);
        for(int j = 0; j < 256; j++) {
            block_num = ind_block[j];
            if(block_num == INODE_FIELD_NO_DATA) {
                free(ind_block);
                free(double_ind_block);
                return blocks;
            }
            blocks[index++] = get_block(block_num);
        }
        free(ind_block);
    }
    VERBOSE_PRINT("Added Double Indirect Blocks\n");
    free(double_ind_block);
    return blocks;
}

/*
 * Returns a NULL-terminated list of the entries for all files in the directory.
 */ 
dir_entry_t **get_dir_entries(inode_t *dir_inode) {
    // TODO test
    VERBOSE_PRINT("Getting directory entries\n");

    void **blocks = get_blocks(dir_inode);
    int entries_per_block = BYTES_PER_BLOCK / sizeof(dir_entry_t);
    int num_entries = dir_inode->file_size / sizeof(dir_entry_t);
    int num_blocks = num_entries / entries_per_block;

    dir_entry_t **entries = calloc(num_entries + 1, sizeof(dir_entry_t*));

    int index = 0;
    dir_entry_t *block_entries;
    for(int i = 0; i < num_blocks; i++) {
        block_entries = (dir_entry_t *) blocks[i];
        for(int j = 0; j < entries_per_block; j++) {
            if(index >= num_entries) {
                free(blocks);
                return entries;
            }
            *entries[index++] = block_entries[j]; // TODO make sure this works
        }
    }

    free(blocks);
    return entries;
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
    // TODO For now only allow for the file to be in the root directory

    // TODO remove this and find actual parent
    int root_inode_block = imap[get_block_key_from_id(ROOT_ID)];
    inode_t *root_inode = get_inode_block(root_inode_block); // TODO free this
    
    inode_t *parent_inode = root_inode; // TODO change this

    /* TODO
     *      1. check that the file is in the root directory for root [get_dir_entries()]
     *      2. use the imap with the dir_entry for the file to find the inode location
     *      3. get the inode block containing the file's [inode get_inode_block()]
     *      4. retrieve the inode for the file get_inode
     *      5. get the blocks for the file [get_blocks()]
     *      6. copy the contents of the blocks into the buffer given (up to its capacity)    
     */ 
    
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

bitvector_t * _get_free_block_list() {
    return free_block_list;
}

bitvector_t * _get_free_inode_list() {
    return free_inode_list;
}

checkpoint_buffer_t * _get_checkpoint_buffer() {
    return checkpoint_buffer;
}

short * _get_imap() {
    return imap;
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