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

A new, even more workable plan!

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
index of the indode within its block. 

This means we will need one inode map entry per block of 16 inodes. There 
are 256 blocks of inodes, and each entry is 2 bytes (the block number) so we
will need a total of 512 bytes for the inode map. This can live in block 
number 3 on disk (which is part of the reserved section).

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

Since we have an inode free list, the only thing left in the imap_t struct was a
short array, so we will just use a short array explicitly as the imap instead of
wrapping it in a struct.

=====
OLD INODE STUFF [DELETE WHEN YOU'RE CONFIDENT THE ABOVE WILL WORK]
=====
Next up was figuring out some things about inodes:
    1. Will they all be statically allocated at spin-up or generated and destroyed 
       dynamically as users create and delete files?
    2. Do we need a free list for inodes as well?
    3. How should the inode map work?
    4. How are we going to assign inode numbers?
    5. How are we going to find a file given its directory entry?
    6a. How many inodes can I actually have?
    6b. How many inodes should I accomodate for? That is, are there other considerations/
        tradeoffs that might cause me to limit the system to use fewer than another
        implementation?
    7. What stuff do we want to keep track of in the inode flag section?

    It took me quite a while to wrap my head around all of these things, but I 
    think I've come up with a workable plan. This scheme gives the following answers:
    
    1.  
        I want to dynamically create them. This reduces initial overhead
        (both time and disk space), and allocations should be lightweight enough
        that doing them on the fly won't hurt us in terms of runtime. As noted 
        elsewhere, it also does not make sense that we would be able to keep the
        whole inode map and all inodes in main memory when our disk is only a few 
        megabytes. That said, if we are okay/will not lose marks for just having
        a copy of the inode map cached, I would prefer to do this and have the map
        point to actual inode_t structures wherever an inode exist and have NULLs
        for where that inode number doesn't actually have an inode.

        TODO Ask yvonne if this would be okay

    2.  We don't need a bit-vector style free list. Instead this information will be
        kept track of via the inode map.
    3. 
        The inode map will map inode numbers to the location of the inode on the 
        disk and tell us how many inodes we are using and what the number of the 
        next one to be used will be. It also lets us identify whether or not a 
        particular inode-number is being used by an inode.
       
        The map will be represented in memory as a struct (imap_t, at least for 
        now) that contains an array "map" of shorts. map[inode number] = the 
        number of the block containing the corresponding inode. The map struct
        will also have additional information like the number of inodes in use,
        and the next available inode. I don't use the map to point directly to 
        inode_t's, because keeping all inodes in the system in memory seems to 
        be unrealistic in a setting where we have such a small disk.

        The map will be stored on disk as a single block containing a sequence 
        of 2 byte block numbers, where the ith pair of bytes gives the block 
        containing the ith inode.

        Entries for inode numbers for which the corresponding inode has not been
        created/has been deleted will be set to a constant value to indicate this.
    4.
        Inode numbers will range from 0 to 4095. When a new file is created
        the free inode with the lowest number will be used.
    5.
        The spec says that a directory entry is 32 bytes:
            1 byte for an inode descriptor
            30 bytes for the file name
            1 byte for a null terminator for the file name
        
        With this scheme, we could specify part of the inode id number, but not 
        the whole thing, as we can only get 256 unique options using one byte. 
        We could still find the appropriate inode with a 100% success rate, but 
        we would need to do some searching. 

        I have decided to deviate a little from the spec here and allow 2 bytes 
        for the inode descriptor in each directory entry in exchange for decreasing
        the maximum file name size from 30 to 29. This allows us to find the exact
        location of the inode via the map on the first index into the map every time.
    6a.
        We are using 2 bytes for the inode numbers, which gives us an initial 
        limit of 2^16. As mentioned in 8, we will be using one bit of this for
        the directory flag, so we're now bounded by 2^15.

        If we consider the case where we have a disk full of empty files (i.e. 
        files which have an inode but no data) then we see that we could 
        accomodate up to 16*4086 = 65376 > 2^15 inodes.

        The inode map is also a limiting factor on the number of inodes we can 
        have. Each mapping takes 2 bytes on disk, so we can have 512/2 = 256 
        entries per block. If we stick to the limit of 10 reserved blocks, 8 of
        which have room for us to work, we can get 256*8 = 2048 inodes. Using all
        of our remaining reserved blocks to get only 2048 inodes feels incredibly
        sub-optimal. Instead, we could add one level of indirection, and have each
        2 byte entry in a reserved block point to a data block. Doing this lets us
        get 256^2 inodes via a single indirect block; however, this might introduce 
        issues with maintaining their ordering in order to track their numbers.

        From all of this, our upper bound is 2^15 = 32768 inodes.
    6b.
        We will have 4096 inodes.

        Another factor at play is how many files we can actually fit on the disk.
        This doesn't lower our number of inodes from part a if we consider the 
        possibility of all files (except root) being empty, but this is a highly 
        unlikely event, and the optimization of the system for large numbers of
        empty files is decidedly unimportant. As such, we consider that the 
        maximum number of files which can be created using one data block (rather
        than zero) is only 3845. 
        Since we can fit 16 inodes per block, we can make 240 sets of 17 blocks
        where 16 are data blocks and the last contains 16 inodes for these data 
        blocks. We are then left with 6 blocks of the available 4086 to use for 
        5 more non-empty files. 
        This means that having 32000 inodes is really quite overkill since I am 
        making the assumption that most files will actually have some content.

        Because of this, I have decided to support only 4096 inodes. To do so, we
        use 1 level of indirection, but we use a relatively small number
        of indirect blocks. 
        We reserve the third (index #2) block on disk for our mapping block.
        the first 32 bytes are used to point to 16 indirect blocks,
        each of which can store 256 block # mappings. This gives a total of 4960
        inode mappings, which should be sufficient for our purposes. 
    7.
        We are going to break up the flag section from the spec into two 16 bit 
        pieces:
            parent_id: The inode number of the parent directory.

            id: The inode number. One bit of this will serve as a flag to tell
                us whether a file is a directory or not.

Preventing us from having sparse inodes:
    - We are going to be writing log changes to an in-memory segment
    - Just put all updated inodes in blocks together at the end of the segment before we write out to disk
    - The logistics of this might get a tad hairy, and we will need to be very careful with the math needed for us
    to ensure that inode pointers are correct, but it will make sure we don't have a bunch of blocks with only one inode in them
    - Updates to the inode map will also be needed here. The reserved blocks can be updated in place or overwritten 
    entirely by just re-formulating the content based on the in-memory inode map

==== 



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