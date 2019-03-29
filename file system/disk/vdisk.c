/* Author: Jamie Kirkwin
 * 
 * This is the lowest level API in the file system assignment
 * 
 * It defines the  operations that can be performed on the virtual disk.
 * Namely: reading and writing a given block.
 * 
 * Here (in the h file) we also define constants for the size of the blocks used
 * and number of blocks on the disk
 */ 

#include <fcntl.h> // open()
#include <unistd.h> // read(), write(), lseek(), close()
#include <stdlib.h>
#include <string.h>

/*
The stuff below is path_unbuffered!
- int open(const char *pathname, int flags); // >> fcntl.h <<
- ssize_t read(int fd, void *buf, size_t count); // unistd.h
- ssize_t write(int fd, const void *buf, size_t count); //unistd.h
- off_t lseek(int fd, off_t offset, int whence); //unistd.h
- void close(int fd); //unistd.h 
*/

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
 * Fills a path_buffer of size 512 that was passed in with the specified block's 
 * contents
 * 
 * @param
 * block_number:    the block number to read from
 * buffer:          a buffer to store the block's content in
 * alt_disk_path:   An optional path to an alternate disk on which to perform 
 *                  the operation. 
 *                  If none is specified, the operation is performed on the    
 *                  vdisk file located in the current directory 
 * 
 * @return true if successful, false otherwise
 * 
 * @pre if no alt_disk_path is provided, there must exist a formatted vdisk in 
 *      the current working directory
 */

bool vdisk_read(int block_number, void *buffer, char *alt_disk_path) {
    if(block_number < 0 || BLOCKS_ON_DISK <= block_number) {
        fprintf(stderr, "Failed read: block number %d out of range\n", block_number);
        return false;
    }
    if(!buffer) {
        fprintf(stderr, "Failed read: No buffer given\n");
    }

    char *vdisk_path;
    if(alt_disk_path) {
        vdisk_path = alt_disk_path; 
    } else {
        vdisk_path = get_vdisk_path();
        if(!vdisk_path) {
            fprintf(stderr, "read failed: could not generate path to disk\n");
            return false;
        }
    }

    int fd = open(vdisk_path, O_RDONLY);
    if(-1 == fd) {
        fprintf(stderr, "read failed: could not open file\n\t\"%s\"\n", vdisk_path);
        return false;
    }
    
    off_t offset = BYTES_PER_BLOCK * block_number;
    lseek(fd, offset, SEEK_SET);
    
    ssize_t bytes_read = read(fd, buffer, BYTES_PER_BLOCK);
    if(bytes_read < 0) {
        fprintf(stderr, "read failed: unable to perform disk read\n");
        return false;
    }
    
    if(!alt_disk_path) {
        free(vdisk_path);
    }
    return true;
}

/*
 * Write the content of a buffer of size 512 to the specified block
 * 
 * @param
 * block_number:    the block number to write to
 * buffer:          a buffer containing the content to write
 * alt_disk_path:   An optional path to an alternate disk on which to perform 
 *                  the operation. 
 *                  If none is specified, the operation is performed on the    
 *                  vdisk file located in the current directory 
 * 
 * @return true if successful, false otherwise
 * 
 * @pre if no alt_disk_path is provided, there must exist a formatted vdisk in 
 * the current working directory
 */

bool vdisk_write(int block_number, void *buffer, char *alt_disk_path) {
    // TODO
    return false;
}