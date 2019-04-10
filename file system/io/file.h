#include "../disk/vdisk.h"

#define MAGIC_NUMBER 1337
#define RESERVED_BLOCKS 10

#define FREE_BLOCK_LIST_BLOCK_NUMBER 1
#define FREE_INODE_LIST_BLOCK_NUMBER 2
#define IMAP_BLOCK_NUMBER 3

#define BITS_PER_BIT_VECTOR 4096 

#define NUM_INODES 4096
#define INODES_PER_BLOCK 16
#define NUM_INODE_BLOCKS 256
#define INODE_FIELD_NO_DATA -1 // 0xFFFF for a short
#define ROOT_ID 0x1000

#define MAX_FILENAME_LENGTH 30

#define CHECKPOINT_BUFF_SIZE 64

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
inode_t* get_inode_block(short block_number);
inode_t *create_inode(int file_size, short id, short parent_id, short* direct,
    short num_direct, short single_ind_block, short double_ind_block);
inode_t *create_empty_inode(short id, short parent_id);


/* =========================== Directories =============================*/

typedef struct dir_entry {
    unsigned char inode_id; // an inode block number (key for imap)
    char filename[MAX_FILENAME_LENGTH + 1]; // Null terminated string 
} dir_entry_t;

/* =========================== Bit Vector API ===========================*/

// Up to the user to update next_available as appropriate
typedef struct free_list_vector {
    unsigned char vector[BITS_PER_BIT_VECTOR];
    short n; // bits marked as unavailable (0)
    short next_available; // the number of the smallest bit currently available
} bitvector_t;

void set_vector_bit(bitvector_t *vector, short index);
bool test_vector_bit(bitvector_t *vector, short index);
void clear_vector_bit(bitvector_t *vector, short index);

void clear_entire_vector(bitvector_t *vector);
void set_entire_vector(bitvector_t *vector);

/* =========================== LLFS API ===========================*/

// Checkpoint Buffer

typedef struct cb_entry {
    int content_length;
    void *content;
} cb_entry_t;

// dirty_inode_list uses same key structure as free_inode_list
typedef struct checkpoint_buffer {
    int blocks_used;
    cb_entry_t* buffer[CHECKPOINT_BUFF_SIZE]; 
    bitvector_t *dirty_inode_list; // Track which inodes correspond to dirty files
} checkpoint_buffer_t;

checkpoint_buffer_t *init_checkpoint_buffer();
void destroy_checkpoint_buffer();
bool add_entry_to_checkpoint_buffer(void* block, int content_length, int inode_id);


// Framework
void init_LLFS(); // Should be called at startup
void flush_LLFS(); 
void defrag_LLFS(); 
void terminate_LLFS(); // Should be called on shutdown

bool is_valid_filename(char * filename);
dir_entry_t **get_dir_entries(inode_t *dir_inode);
void *get_block(short block_number);
void **get_blocks(inode_t *inode);

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
short * _get_imap();
bitvector_t * _get_free_block_list();
bitvector_t * _get_free_inode_list();
checkpoint_buffer_t * _get_checkpoint_buffer();