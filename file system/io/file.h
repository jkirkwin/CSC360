#include "../disk/vdisk.h"

#define NUM_INODES 65376
#define MAGIC_NUMBER 1337

typedef struct inode {
    unsigned int file_size;
    
    // int id; // Also used to indicate whether file is a directory
    // Alternatively, this id can simply be called flag and the inode number 
    // can be the index/key of the inode in the inode map.

    // could also split this up into two parts: two bytes for the id and two
    // to be used for flags
    short id;
    short parentid;
    // we could also split this up to have a pointer to the parent dir in 2 
    // bytes and use the other 2 for the inode number, provided the dir flag is
    // integrated into this number (i.e. uses one of these bits)

    short direct[10]; // Each one of these is a block number, not the disk address 
    short single_ind_block; // the block number, not the actual disk address 
    short double_ind_block; // the block number, not the actual disk address
} inode_t;

// Free list API
void set_free_list_bit(short block_num);
bool test_free_list_bit(short block_num);
void clear_free_list_bit(short block_num);

// LLFS API
void initLLFS(FILE* alt_disk);