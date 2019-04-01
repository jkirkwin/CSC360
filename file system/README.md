# CSC 360 Assignment 4: Little Log Structured File System (LLFS)

__TODO__ <br>
* Make a makefile
* Figure out cunit so that you don't have an awkward inconsistently implemented test suite accross the project 


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

---

<!-- ## APIs & Notes -->

## vDisk

__Design Question:__

    should we use buffered or unbuffered IO for our disk API?
    my gut says go with unbuffered to mitigate the possibilty of crash failures.
    That is what I will do


* We use a file called "vdisk" as a virtual disk for this assignment
* The vdisk is located in /apps/
* Deleting the vdisk lets us start from scratch (might be useful for some testing - could potentially parameterize which vdisk we're using for a given opperation - could help with test modularity)

Block size : 512 bytes <br>
Number of blocks on disk : 4096 (numbered 0 through 4095)

__Block 0 : SuperBlock__

    First 4 bytes : LLFS magic number (shows that the current formatting is LLFS)
    Second 4 bytes: number of blocks on disk
    Third 4 bytes : number of inodes on disk

__Block 1 : Free block bit vector__
* Bits 0 - 9 are always 0 (reserved)
* Bits 10 - 4095 are flags to show whether the corresponding block is free

__Blocks 2 through 9__

* Reserved for whatever you like
* Use this for an inode map (mapping inode number to the block which contains it)
    * This must contain root dir's inode!

### __```/disk/vdisk.c```__

```bool read(int block_number, void *buffer, char *alt_disk_path)```

Use

    Fills a buffer (passed as a param) of size 512 that was passed in with the specified block's contents

Params

    1. the block number to read from
    2. a buffer to store the block's content in
    3. An optional path to an alternate disk on which to perform the operation. 
       If none is specified, the operation is performed on the vdisk file located in the current directory 

    
Return

    true if successful, false otherwise

```bool write(int block_number, void *buffer, char *alt_disk_path)```

Use:

    Writes the specified buffer content to the vdisk at the block number specified

Params:

    1. the block number to write to
    2. a 512 byte argument (or a pointer to such a buffer)
    3. An optional path to an alternate disk on which to perform the operation. 
       If none is specified, the operation is performed on the vdisk file located in the current directory 

Return: 

    true if successful, false otherwise    


---

## LLFS

```int initLFS(char* alt_disk_path)```

Note:

    I changed from spec to start with a lowercase 'i' instead of being horrible

Use:

    The disk is wiped and formatted appropriately as a brand new disk

Params:

    1.  An optional path to analternate disk on which to perform the operation. 
        If none is specified, the operation is performed on the vdisk file located in the current directory 

Return:

    true if successful, false otherwise    


```function_signature```

Use

    Description of function

Params

    Parameters go here

Return

    Return value description goes here


## My "Journey"

I ran into an issue part way through creating my disk controller: my machine 
decided to stop recognizing includes to libraries like stdio, stdlib, and stdbool

I decided to just power through and do all the compilation and testing on the 
ssh server, pulling from my github repo for this class to get the source files
I wrote on my windows machine. 

I then noticed that including fcntl.h, needed for unbuffered opening of my vdisk
failed on the ssh server! I really wanted to use unbuffered operations for the 
virtual disk because it seemed much more true to the concept of trying to 
mimic a physical disk as closely as possible. 

At this point I am unsure whether I will try to debug this or resort to using 
buffered IO in my disk controller. I made a post on the forum showing my error
from the include; hopefully someone is able to help!

Yvonne commented on my forum post saying that I should just use a buffered 
approach, so I'll just do that.

Re-wrote vdisk_read() to use buffered operations instead.

In my infinite wisdom, I decided to test out my manual_read() function used for 
my disk unit tests by reading my source file for the disk controller. It wiped 
the file and I haven't committed since I re-wrote it with buffered operations. I
am ready for death.

Read now seems to work; the tests for it are passing. Something weird is going 
on with write though: a single write appears to work, but when doing multiple 
writes only the last one takes effect. My first guess was that this is because
our writes are buffered, so I tried flushing the stream after each write but 
this did not change the behavior :(. The next idea (thanks to my roommate Jon!)
is that each write is actually successful but the whole file is getting 
overwritten each time we write a block. After a little probing, it looks like 
each one is successful in writing the intended block, but all else gets zero-ed
out! Now to see if we can figure out why.

Bingo: for some reason using "wb+" still overwrites the file. Solved by opening 
in "rb+" mode instead. Unit tests for disk read and write passing on windows.

I'm going to be moving on to file.c next. It looks like its development will be 
coupled to that of the LLFS init() function, so we will place it here instead of
in the disk controller as I initially planned.

The first structure I implemented is the bit vector and api for the free list. I
used http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
as a guide, but I used chars instead of ints because using a single byte at a 
time felt more natural for this application. This API assumes that the current 
free block vector in memory is correct. It does not involve the disk at all (yet). 

Key question: Is it reasonable to make the restriction that every file must have
a data block on creation? Answer: it is reasonable but unnecessary. With the 
restriction, we need 2 bytes to uniquely identify an inode which is the same as
without the restriction, but this gives a little more freedom. In the worst case
(i.e. the case with the most inodes) we have no data blocks, and a disk full of 
dense inode blocks. There are 4086 blocks available and we can get 16 inodes in 
each block so we can bound the number of inodes above by 4086*16 = 65376. With 2
bytes per inode number, we can uniquely identify all of these inodes. 

We need to be able to point to the right inode entry for a file from a 
directory entry. The specification says that each entry is 32 bytes, with 
the first used for the inode number and the next 31 for a null terminated 
filename. With one byte we can identify the block in which the correct inode
is stored, but we cannot identify which inode it is with 100% accuracy as it
is possible that two files might exist such that their inodes are in the 
same block, they are in different directories, and they have the same file 
name. To solve this, I plan to change the directory entry structure to have 
filenames one character shorter (a small concession) in exchange for being able
to easily and quickly identify the correct inode given a directory entry.