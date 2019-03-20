# CSC 360 Assignment 4: Little Log Structured File System (LLFS)

## Info/notes to integrate


### inodes
* there is one for each file
* each has 10 direct blocks of size 2 bytes
    * these give the block number on the disk
* each has two indirect blocks, one with single indirection and one this double indirection
    * each list of addresses for these takes up an entire block

### State
* we use a bit vector stored in the superblock to tell us which blocks on disk are used and which are free
    * bit ```i``` set to 0 <==> block ```i``` on disk is in use 

----------------

<!-- ## APIs & Notes -->

## vDisk

* We use a file called "vdisk" as a virtual disk for this assignment
* The vdisk is located in /apps/
* Deleting the vdisk lets us start from scratch (might be useful for some testing - could potentially parameterize which vdisk we're using for a given opperation - could help with test modularity)

Block size : 512 bytes <br>
Number of blocks on disk : 4096 (numbered 0 through 4095)

__Block 0 : SuperBlock__

    first 4 bytes: LLFS magic number (shows that the current formatting is LLFS)
    second 4 bytes: number of blocks on disk
    third 4 bytes: number of inodes on disk

__Block 1 : Free block bit vector__
* Bits 0 - 9 are always 0 (reserved)
* Bits 10 - 4095 are flags to show whether the corresponding block is free

__Blocks 2 through 9__

* Reserved for whatever you like
* Use this for an inode map (mapping inode number to the block which contains it)
    * This must contain root dir's inode!



```InitLFS()```
    
    vdisk is wiped and formatted appropriately as a brand new disk

```int read(int block_number, void *buffer)```

Use

    Fills a buffer (passed as a param) of size 512 that was passed in with the specified block's contents    

Params

    the block number to read from
    a buffer to store the block's content in
    
Return

    true if successful, false otherwise

```int write(int block_number, void *buffer)```

    takes a block number to write to
    takes a 512 byte argument (or a pointer to such a buffer)

    writes the specified buffer content to the vdisk at the block number specified.

    Question: do we needint to include an offset thing here? That seems out of scope for a disk utility, should probably be handled by the file system

    return true if successful, false otherwise    



---------------------------------

```function_signature```

Use

    Description of function

Params

    Parameters go here

Return

    Return value description goes here