#include <stdio.h>
#include <string.h>
#include <stlib.h>

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

bool vdisk_read(int block_number, void *buffer, FILE *alt_disk) {
    FILE fp;
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
    int bytes_read = fread(buffer, BLOCK_SIZE, 1, fp);
    if(bytes_read != BLOCK_SIZE) {
        fprintf(stderr, "Read Failed: Read %d bytes (should be %d)\n", 
            bytes_read, BYTES_PER_BLOCK);
    } 

    if(!alt_disk) {
        free(path);
        fclose(fp);
    }
}

// Offset is where to start writing in the block. Method will only write to the 
// end of the specified block
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

    FILE fp;
    char *path;
    if(alt_disk) {
        fp = alt_disk;
    } else {
        path = get_vdisk_path();
        fp = fopen(path, "wb");
        if(!fp) {
            fprintf(stderr, "Write Failed: cannot open vdisk\n");
            return false;
        }
    }

    fseek(fp, BYTES_PER_BLOCK * block_number + offset, SEEK_SET);
    int length = BLOCK_SIZE - offset;
    if(content_length < length) {
        length = content_length;
    }
    int bytes_written = fwrite(buffer, content_length, 1, fp);
    if(bytes_written != length) {
        fprintf(stderr, "Write Failed: Wrote %d bytes (should be %d)\n", 
            bytes_written, length);
    } 

    if(!alt_disk) {
        free(path);
        fclose(fp);
    }
}