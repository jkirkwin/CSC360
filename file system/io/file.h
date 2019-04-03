#include "../disk/vdisk.h"

#define NUM_INODES 4096
#define INODES_PER_BLOCK 16
#define NUM_INODE_BLOCKS NUM_INODES / INODES_PER_BLOCK
#define MAGIC_NUMBER 1337
#define MAX_FILENAME_LENGTH 30

/* ========================= Inodes and directories =========================*/
typedef struct inode {
    unsigned int file_size;
    short id; // Inode number: 1 bit for dir flag, 4 for offset, 8 for imap block key
    short parent_id;
    short direct[10]; // Each one of these is a block number, not the disk address 
    short single_ind_block; // the block number, not the actual disk address 
    short double_ind_block; // the block number, not the actual disk address
} inode_t;


typedef struct dir_entry {
    unsigned char inode_id; // an inode block number (key for imap)
    char filename[MAX_FILENAME_LENGTH + 1]; // Null terminated string 
} dir_entry_t;

/* =========================== Free list API ===========================*/

// Up to the user to update next_available
typedef struct free_list_vector {
    unsigned char vector[512];
    short n; // bits marked as unavailable (0)
    short next_available; // the number of the smallest bit currently available
} bitvector_t;

void set_vector_bit(unsigned char* vector, short index);
bool test_vector_bit(unsigned char* vector, short index);
void clear_vector_bit(unsigned char* vector, short index;

/* =========================== LLFS API ===========================*/
void initLLFS(FILE* alt_disk);