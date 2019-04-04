#include "../disk/vdisk.h"

#define MAGIC_NUMBER 1337
#define RESERVED_BLOCKS 10

#define BITS_PER_BIT_VECTOR 4096 

#define NUM_INODES 4096
#define INODES_PER_BLOCK 16
#define NUM_INODE_BLOCKS 256
#define INODE_FIELD_NO_DATA -1

#define MAX_FILENAME_LENGTH 30

/* ========================= INode API =========================*/

typedef struct inode {
    unsigned int file_size;
    short id; // Inode number: 1 bit for dir flag, 4 for offset, 8 for imap block key
    short parent_id;
    short direct[10]; // Each one of these is a block number, not the disk address 
    short single_ind_block; // the block number, not the actual disk address 
    short double_ind_block; // the block number, not the actual disk address
} inode_t;

inode_t *create_inode(int file_size, short id, short parent_id, short* direct,
    short num_direct, short single_ind_block, short double_ind_block);
bool is_dir(short inode_id);
unsigned char get_block_key_from_id(short inode_id);
short generate_inode_id(bool is_dir);


/* =========================== Directories =============================*/

typedef struct dir_entry {
    unsigned char inode_id; // an inode block number (key for imap)
    char filename[MAX_FILENAME_LENGTH + 1]; // Null terminated string 
} dir_entry_t;


/* =========================== Free list API ===========================*/

// Up to the user to update next_available as appropriate
typedef struct free_list_vector {
    unsigned char vector[BITS_PER_BIT_VECTOR];
    short n; // bits marked as unavailable (0)
    short next_available; // the number of the smallest bit currently available
} bitvector_t;

void set_vector_bit(bitvector_t *vector, short index);
bool test_vector_bit(bitvector_t *vector, short index);
void clear_vector_bit(bitvector_t *vector, short index);

/* =========================== LLFS API ===========================*/

void initLLFS(FILE* alt_disk);