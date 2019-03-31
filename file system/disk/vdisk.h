#include <stdio.h>
#include <stdbool.h>

#define BYTES_PER_BLOCK 512
#define BLOCKS_ON_DISK 4096 

bool vdisk_read(int block_number, void *buffer, FILE *alt_disk);
bool vdisk_write(int block_number, void *buffer, FILE *alt_disk);