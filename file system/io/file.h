#include "../disk/vdisk.h"

#define NUM_INODES 65376
#define MAGIC_NUMBER 1337

typedef struct inode {
    int file_size;
    int id; // Also used to indicate whether file is a directory
    // Alternatively, this id can simply be called flag and the inode number 
    // can be the index/key of the inode in the inode map.

    // could also split this up into two parts: two bytes for the id and two
    // to be used for flags
    short direct[10];
    short single_ind;
    short double_ind;
} inode_t;

// Free list API
void set_free_list_bit(short block_num);
bool test_free_list_bit(short block_num);
void clear_free_list_bit(short block_num);

// LLFS API
void initLLFS(FILE* alt_disk);