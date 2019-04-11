#include <stdlib.h>
#include <stdio.h>

#include "../disk/vdisk.h"
#include "../io/file.h"

/*
 * As per the spec, this file showcases the functionality of creating and 
 * mounting a disk. For the unit tests, see disk/test_vdisk.c and io/test_file.c
 */ 


int main() {
    printf("\n-----Test02-----\n");
    // Note that in order for this to make sense, there should be no vdisk in the cwd

    printf("Writingto create the disk\n");
    vdisk_write(0, "abc", 0, 3, NULL);

    printf("mounting vdisk with call to init_LLFS()\n");
    init_LLFS();

    int superblock_buff[512/sizeof(int)];
    vdisk_read(0, superblock_buff, NULL);
    if(superblock_buff[0] != MAGIC_NUMBER) {
        printf("Magic Number not found, disk mount failed\n");
    }
    printf("Magic number verified; disk creation and mount successful\n");
    printf("Test complete.\n");
}