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

### The disk
I ran into an issue part way through creating my disk controller: my machine 
decided to stop recognizing includes to libraries like unistd, stdio, stdlib, 
and stdbool. 


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

### Starting the File System: Free List & Inodes

Note: I have named my filesystem library "file.c" not "File.c" because I am not 
an animal who mixes case conventions.

The first structure I implemented is the bit vector and api for the free block list. I
used http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
as a guide, but I used chars instead of ints because using a single byte at a 
time felt more natural for this application. This API assumes that the current 
free block vector in memory is correct. It does not involve the disk at all (yet). 
I have not yet abstracted it so I can reuse this code for an analogous inode free list,
if that ends up being a part of my system.

RECURSIVE UPDATE PROBLEM (ref http://pages.cs.wisc.edu/~remzi/OSTEP/file-lfs.pdf)
We cannot use my idea of having inode numbers being tied directly to their disk location, as this
necessitates updates to the directory containing the corresponding entry, which then causes us to
have to update all directories in the path from the root to the cwd. See excerpt below for a 
more specific explanation. 

    "There is one other serious problem in LFS that the inode map solves,
    known as the recursive update problem. The problem arises
    in any file system that never updates in place, but rather
    moves updates to new locations on the disk.
    Specifically, whenever an inode is updated, its location on disk changes.
    If we hadn’t been careful, this would have also entailed an update to
    the directory that points to this file, which then would have mandated
    a change to the parent of that directory, and so on, all the way up the file
    system tree.
    LFS cleverly avoids this problem with the inode map. Even though
    the location of an inode may change, the change is never reflected in the
    directory itself; rather, the imap structure is updated while the directory
    holds the same name-to-inode-number mapping. Thus, through indirection, 
    LFS avoids the recursive update problem."

I am doing my best to stick to the spec, and in the directory entry 
description it says that only 1 byte will be used to identify the inode
corresponding to a given file in that directory.

Initially I assumed that this single byte would be the inode number, which
would limit us to 256 inodes. This seemed like a very small maximum 
allowence. If instead, we go with static allocation of all of our inodes to 
ensure that there are always 16 per data block, we can allow for 256 blocks
instead! Each block can hold 16 inodes, so we get a maximum number of 4096
inodes instead. This does come with the space overhead of making 256 data
blocks unavailable for inodes which may be unused, but it allows many more 
inodes and ensures that when we do have a large number of files, the inodes
are guaranteed to be stored in a maximally compact way without the need for
complex logic and overhead during cleanup.

With the approach, inode numbers will be 2 bytes. 8 bits allow us find the 
block containing the inode via the inode map, and 4 other bits give the 
index of the indode within its block. We also store the directory flag in one of
the other 4 bits.

This means we will need one inode map entry per block of 16 inodes. There 
are 256 blocks of inodes, and each entry is 2 bytes (the block number) so we
will need a total of 512 bytes for the inode map. This can live in block 
number 3 on disk (which is part of the reserved section).

Inodes are numbered from 0 to 4095, and their id entries are of the form:
[Dir Flag][Unused][Unused][Unused] [Block #][Block #][Block #][Block #] (high byte) 
[Block #][Block #][Block #][Block #] [Offset][Offset][Offset][Offset] (low byte)

Thus, by clearing the dir flag and 3 unused bits, we get the index of the inode
in the free list.

Block number 2 will contain an inode free list analogous to the block free 
list detailed above. It will be a bit vector of size 4096 (one for each 
inode). This means that it will take up one full block.

Since we have the potential for a whole lot more than 256 indoes now, we will 
need an inode cache for the file system to use, as we can't just pull all 4096 
in to memory (in the worst case). This will need to be implemented fairly simply
if for no other reason than that I don't have a whole lot of time to do this 
assignment. Most likely, I will end up implementing this as a FIFO array based 
queue of inode structs. 
This is a TODO for when I have gotten further with the 
implementation of the rest of the inodes stuff and have a better feel for how
best to do it.

We will use one of the remaining bits in the 2 byte inode number to indicate 
whether the corresponding file is a directory.

We will use our last 2 bytes inside each inode for the parent directory's inode
number. This will give us an easy way to ensure that we are 100% accurate when
looking up some file within a directory. The case in question is if two inodes
in the same block correspond to files with the same name with different parent
directories. This way we won't choose the wrong one.

### Implementing Inode Stuff

The first thing to do is to re-factor my free list bitvector from before to be a
generic 1 block bit vector so I can use it for both block and inode freelists.

    Created a bitvector_t struct for these and changed the API accordingly.
    Unit test was modifed and still passes

Since we have an inode free list, the only thing left in the imap_t struct was a
short array, so we will just use a short array explicitly as the imap instead of
wrapping it in a struct.

Started working on the init function again, but we need some inode and directory
support first, since we need to actually create the root directory.

    Added functions for
        - making inode_t structs
        - generating an inode id to be used for a new inode
        - determining if an inode is a directory 
        - extracting an inode's block number (imap key) from its id
        - extracting an inode's offset given its id

Verifed that tests pass for all of them on windows and the ssh server.

The next thing to do is create the root directory. This will entail setting up an
inode for it, adding this to the inode map, and adding this to the inode free list.
We may also decide to add a data block for the directory immediately, as any change 
to the file system (i.e. creating a new file) will entail creating one for root. 
All this will need to be written to disk immediately, as we don't want a dirty log
right after initializing; that's just gross.

    In the process of doing this, I ran the init function for the first time and
    noticed something not so great: I have a loop to write every block of the 
    disk with zeros, but this takes about 22 seconds! Instead of doing this 
    manually and bypassing the disk api (which we wouldn't be able to do in real
    life) I've decided to just not zero out the whole thing. As long as the data
    structures are correct on disk and in memory this should not pose any issues.

    I added the root creation functionality and added writes to disk. A few bugs
     were found while probing for sanity checks, now for the actual testing! (TODO)

[TODO]
Once we have this done, we need to implement the test for init_LLFS and then we 
can move on to support writing to files. For this we will need to come up with an
API for our file system (read and write for now should be sufficient). We also 
need a checkpoint buffer in which to store changes. At its heart, this should be
an array of blocks, in the order that we would like to write them. We can choose 
to write this buffer to disk when it gets full, but we need to be careful of only 
sending part of some transaction to disk. A better way to go would be to check before
starting each write whether it will cause the buffer to fill/overflow. If so, write the 
current content (and update the inode map, inode free list, and block free list), clear
the buffer and continue. Another thing we need to watch out for is single transactions
which can cause the buffer to overflow (i.e. writing a file larger than our buffer). In
this case, the best way to go seems to be to split this up into separate transactions.

[TODO] a potentially useful utility would be a manual "flush" - i.e. forcing us to 
push all changes etc to disk and make things consistent. This would be useful in init as
well as in the case above and when implementing a cleanup/teardown function.
  

[TODO] It is also worth noting here that we need to make sure that root cannot 
be deleted. 

### Misc Notes

When removing an inode, make sure to free up any indirect blocks allocated.

Checkpointing:
    - need a segment buffer to be sent to disk when full (this is a collection of
    new stuff to add to the end of the log) 
    - Need a way to ensure this is written before a calling process terminates so we don't lose progress
            - One way to do this is to put the onus on the user to perform a manual
              checkpoint before ending a program which uses this library 
              (essentially like closing a file system) 

We need to make sure that we're populating our data structures in memory properly
from disk before users are able to actually use the file system. One simple way to do
this is to define a user API which is a subset of the functions written here (essentially
everything that would be made public were we doing this in Java) and in each of these fuctions
check whether we have done this population (if not, do it then).
Alternatively, we could ask that the user call a setup function before using the library. This
falls in line nicely with a cleanup-style function mentioned in the checkpointing note above. 