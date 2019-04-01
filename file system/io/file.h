typedef struct sb_content {
    int magic_number;
    int blocks_on_disk;
    int inodes_on_disk;
} sp_content_t;

typedef struct inode {
    int file_size;
    int id; // Also used to indicate whether file is a directory
    // Alternatively, this id can simply be called flag and the inode number 
    // can be the index/key of the inode in the inode map.
    short direct[10];
    short single_ind;
    short double_ind;
} inode_t;

// Free list api
void set_free_list_bit(short block_num);
bool test_free_list_bit(short block_num);
void clear_free_list_bit(short block_num);