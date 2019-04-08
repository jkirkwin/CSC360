#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "vdisk.h"
/*
 * A helper function for getting path to vdisk in current directory.
 * 
 * @return the path to the current directory plus '\vdisk' or NULL if there is
 *         an error.
 */ 
char *get_vdisk_path() {
    size_t buff_size = 20;
    char *path_buffer = (char *) malloc(buff_size * sizeof(char));
    if(!path_buffer) {
        fprintf(stderr, "Failure getting vdisk path: malloc failed\n");
        return NULL;
    }
    char * ret = getcwd(path_buffer, buff_size);

    while(!ret) {
        buff_size = buff_size * 2;
        path_buffer = (char *) realloc(path_buffer, buff_size);
        if(!path_buffer) {
            fprintf(stderr, "Failure getting vdisk path: malloc failed\n");
            return NULL;
        }
        ret = getcwd(path_buffer, buff_size);
    }
    
    // getcwd() does not terminate with a '/'
    char *vdisk_path = malloc(sizeof(char) * (buff_size + strlen("/vdisk\0")));
    if(!vdisk_path) {
        fprintf(stderr, "Read failed: malloc failed\n");
        return NULL;
    }
    
    strncpy(vdisk_path, path_buffer, buff_size);
    strncat(vdisk_path, "/vdisk\0", strlen("/vdisk\0"));
    free(path_buffer);
    return vdisk_path;
}

/*
 * Reads one block into the buffer provided. Blocks are of size 512 bytes, and 
 * the buffer should be this large as well.
 * 
 * Blocks are indexed from 0 to 4095.
 * 
 * Alt disk must be opened for reading in binary mode
 * 
 * If no alt_disk is provided, this reads from the vdisk in the current working
 * directory
 */ 
bool vdisk_read(int block_number, void *buffer, FILE *alt_disk) {
    FILE *fp;
    char *path;
    if(alt_disk) {
        fp = alt_disk;
    } else {
        path = get_vdisk_path();
        fp = fopen(path, "rb");
        if(!fp) {
            fprintf(stderr, "Read Failed: cannot open vdisk\n");
            return false;
        }
    }

    fseek(fp, BYTES_PER_BLOCK * block_number, SEEK_SET);
    int items_read = fread(buffer, BYTES_PER_BLOCK, 1, fp);
    if(items_read < 1) {
        fprintf(stderr, "Read Failed: fread() unsuccessful\n"); 
        return false;
    } 

    if(!alt_disk) {
        free(path);
        fclose(fp);
    }
    return true;
}


/*
 * Writes one block from the buffer provided. Blocks are of size 512 bytes, and 
 * the buffer should be this size or less.
 * 
 * Blocks are indexed from 0 to 4095.
 * 
 * Alt disk must be in rb+ mode
 * 
 * If no alt_disk is provided, this reads from the vdisk in the current working
 * directory
 * 
 * Offsetting is supported, but content will be truncated in the case that it
 * cannot fit into one block given the offset.
 */ 
bool vdisk_write(int block_number, void *content, int offset, int content_length,
         FILE *alt_disk) {

    if(block_number < 0 || block_number >= BLOCKS_ON_DISK) {
        fprintf(stderr, "Write failed: invalid block number: %d\n", block_number);
        return false;
    }
    if(!content) {
        fprintf(stderr, "Write failed: no content passed\n");
        return false;
    }
    if(offset < 0 || offset >= BYTES_PER_BLOCK) {
        fprintf(stderr, "Write failed: offset out of bounds. Offset given:%d\n", 
            offset);
        return false;
    }
    FILE *fp;
    char *path;
    if(alt_disk) {
        fp = alt_disk;
    } else {
        path = get_vdisk_path();
        fp = fopen(path, "rb+");
        if(!fp) {
            fprintf(stderr, "Write Failed: cannot open vdisk\n");
            return false;
        }
    }   
    fseek(fp, BYTES_PER_BLOCK * block_number + offset, SEEK_SET);
    int length = content_length;
    if(content_length > BYTES_PER_BLOCK - offset) {
        int difference = content_length - (BYTES_PER_BLOCK - offset);
        fprintf(stderr, "WARNING: Truncated content by %d bytes on disk write to block %d\n", difference, block_number);
        length = BYTES_PER_BLOCK - offset;
    }
    int items_written = fwrite(content, length, 1, fp);
    if(items_written < 1) {
        fprintf(stderr, "Write Failed: fwrite() failed\n");
        return false;
    }
    fflush(fp); 
    if(!alt_disk) {
        free(path);
        fclose(fp);
    }
    return true;
}