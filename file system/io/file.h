#include "../disk/vdisk.h"

#define MAGIC_NUMBER 1337
#define RESERVED_BLOCKS 10

#define BITS_PER_BIT_VECTOR 4096 

#define NUM_INODES 4096
#define INODES_PER_BLOCK 16
#define NUM_INODE_BLOCKS 256
#define INODE_FIELD_NO_DATA -1
#define ROOT_ID 0x1000

#define MAX_FILENAME_LENGTH 30

// From assignment 2
#define VERBOSE // TODO Add as a compilation option instead of toggling here 

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

/* ========================= INode API =========================*/

typedef struct inode {
    unsigned int file_size;
    short id; // Inode number: 1 bit for dir flag, 4 for offset, 8 for imap block key
    short parent_id;
    short direct[10]; // Each one of these is a block number, not the disk address 
    short single_ind_block; // the block number, not the actual disk address 
    short double_ind_block; // the block number, not the actual disk address
} inode_t;

bool is_dir(short inode_id);
short get_inode_free_list_key(short inode_id);
unsigned char get_block_key_from_id(short inode_id);
unsigned char get_offset_from_inode_id(short inode_id);
short strip_dir_bit(short inode_id);
short generate_inode_id(bool is_dir);
inode_t *create_inode(int file_size, short id, short parent_id, short* direct,
    short num_direct, short single_ind_block, short double_ind_block);


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

// Framework
void init_LLFS(); // Should be called at startup
void flush_LLFS(); 
void terminate_LLFS(); // Should be called on shutdown

// Create
inode_t *create_file(char *filename, char *path_to_parent_dir); 
inode_t *mkdir(char * dirname, char *path_to_parent_dir); 

// Write
int write(void* content, int content_length, int offset, char *filename); // can be used to append
int append(char *content, int content_length, char *filename); 

// Read
char **get_dir_contents(char *dirname); 
int read_file(void *buffer, int buffer_size, char *filename); 

// Delete
bool rm(char *filename);

// Misc
inode_t* find_dir(char* dirpath);

/* =========================== Testing ===========================*/

bitvector_t* _init_free_inode_list();
void print_inode_details(inode_t* inode);