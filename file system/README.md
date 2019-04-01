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

The first structure I implemented is the bit vector and api for the free block list. I
used http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
as a guide, but I used chars instead of ints because using a single byte at a 
time felt more natural for this application. This API assumes that the current 
free block vector in memory is correct. It does not involve the disk at all (yet). 
I have not yet abstracted it so I can reuse this code for an analogous inode free list.

Note: I have named my filesystem library "file.c" not "File.c" because I am not 
an animal who mixes case conventions.

------------- TODO, figure out inode mapping and procedure
Determining the number of inodes to use:
    Here there are a few considerations. First, we don't want to eat up too much 
    of the disk with a giant bit vector for the inode free list. Second, we 
    don't want to have a low limit on the number of files that can exist in the
    system. Third, we want mechanics to be reasonably simple from a programming 
    perspective. I had planned to allow for the maximum number of inodes to be 
    4086 * 16 = 65376, where we simply have a disk full of inode-only blocks; 
    i.e. there are 65376 empty files. Alternatively, we can make it so that a
    file must have a data block, even if it is of size zero. This gives much 
    fewer possible inodes (3845), but allows the free list to fit in one block 
    and the programming mechanics to be simple. This feels like too low a limit,
    so I have decided to make a comprimise: use 5 blocks for the inode map, 
    giving us 

Key question: Is it reasonable to make the restriction that every file must have
a data block on creation? Answer: yes. With the 
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

---------------------------------------
QUESTIONS About inodes

1. On init (disk formatting) should I allocate all possible inodes in the first 
few blocks of the disk?
    - If so, we need to track which inodes are free and which are in use with a 
    bit vector.
    - If not, they will just be allocated when new files are created. In this case,
    it would probably make the most sense for us to have inode numbers simply be generated 
    sequentially. It seems like here too we will need a bit vector showing which inodes
    are available, so we can find out if we are out of space, and to find available
    ones if they exist (just do a linear search until one is found)
    - In either case, in addition to having a record of which inodes are free/in use,
    we will also need a way to locate inodes within the disk. The could be achieved 
    fairly easily simply by having a map with key: inode number (unique id) and value: 
    the number of the block which contains the inode.
    
    Decision: whether we go with dynamic or static inode allocation at format-time,
              inode numbers should be contiguous and immutible. That is, inode numbers 
              should only be in some range [0, MAX_INODE_INDEX]. If we go with static 
              allocation of inodes, every index in this range will correspond to exactly one 
              inode stored on disk (and possibly copied in memory, but that's another 
              kettle of fish).

    This gives us a little guidance on how many inodes we should allow. It also 
    seems to follow (by my gut instinct at least) that we should also have a mapping
    existing in the first 10 reserved blocks on disk which maps inode numbers to the 
    blocks containing them. Such a mapping would need 2 bytes for the value, which is 
    a block number. The number of bytes needed for the key is determined by the number
    of inodes allowed in the system.

    1 byte per inode number allows for only 256 inodes. 2 bytes allow for up to 65536 inodes. 
    This is not a reasonable number to have if we have a bit vector, as it would need 16 blocks
    alone, without even considering the space required for the mapping of these inodes to their
    blocks on disk.

    As per Yvonne's advice in class, we should not get into the mess of trying to use bits from 
    one byte for extra storage unless we can do so very simply. As such, we likely won't want anything
    too fancy beyond just using the two bytes for inode numbers.

    We then have one bit needed for each inode allowed due to the free list, plus 
    2 bytes for the block containing it and 2 more for its number. We could, however,
    use a mechanic where we use one of the bits in the inode number as a flag to 
    indicate whether it is a directory, and ignore that bit when actually calculating the
    inode number. 

    Given an inode number, we can tell whether it is in use based on the bit vector.
    Given an inode we can tell whether it is in use by examining the first direct pointer.
    For example, we can have the convention that this pointer will be set to -1 if there is no
    file associated with the inode. An issue from this is that it does not work if we
    decide to allow empty files to have no data block. this seems like an unnecessary 
    consideration considering the spec's description of the process of creating a file
    as a write to a file that does not yet exist.

    This leads me to think that it is okay for us to add the constraint that every 
    file must have at least one data block.

    If we go with the approach outlined above, we can see that for each inode in 
    the system, we will need 2 bytes for the inode number and directory flag, and 
    one bit to track whether the inode is in use.

    Another idea to consider is that we still have 2 bytes left in the inode flag
    area to store something, and this could be the parent directory's inode number.
    In that case, we would be able to traverse upwards in the dir tree, and would 
    eliminate the need for the extra byte we were planning on taking from the filename
    in directory entries to make sure we get the right inode. This is because we 
    can tell which block to go to from the map in the reserved area of the disk, and 
    there can exist only one (the one we want) file with the correct parent directory and
    filename combo in that block.

    If we go forward with the strategy, and subscribe to the constraint that we only
    want to reserve 10 blocks of the disk in total, then we can see that our maximum 
    number of inodes is 896. 
            
            let x = #inodes and suppose x < the number of bits in a block = 4096 
            (this is to make sure that only one block is used for the free list)

            then 4x = the number of bytes needed for the physical map

            we have 7 blocks remaining of our 10 reserved ones, so 
            4x/7 <= 512

            thus x <= 512*7/4 = 896

    Idea: What if we merged the inode free list with the inode map?
        i.e. fit the free bit into the map and do away with the separated bit vector...

        alternative: don't use a free list at all: just use the inode map as a list
        of all inodes in use - we just need to ensure that the size of the map does
        not excede the number of entries allowed.   

            the whole inode map gets cached in memory, so if we have a way to determine where the inode map
            ends, we don't need to explicitly store the size on disk (we can however store it in memory)
            One way to do this is define an invalid inode number which indicates the end of this list.
            each map entry will be 4 bytes: 2 for 

            if we go this route then we would need a way to tell how

            ONE LEVEl OF INDIRECTION:
                what if the inode map is actually set up as follows:
                    a subset of our reserved disks contain pointers (2 bytes) 
                    to blocks in memory. These blocks are what actually contain 
                    the mapping entries. I.e. we have some list of blocks in our reserved section,
                    and each of these blocks contains a list of inode numbers and their corresponding
                    block locations (and potentially their offset within some inode block)

                    we would then allow ourselves to have a larger number of inodes
                    we've got three bounding factors for this (so far at least): 
                        1) The number of inodes which can be represented with our inode numbering conventions
                                - with two bytes we are bounded at 65536.
                                - if we go with the idea of using one of these bits to indicate whether
                                this each file is a directory then we have 15 bits left to use, bringing 
                                the bound down to 32768
                                - if we go even further and adopt the idea of using another bit to indicate if the 
                                given inode is actually in use (note that this is only even an option if we are 
                                statically allocating inodes off the bat). then we can go up to 2^14 = 16384

                        2) The number of inodes which can be mapped with our scheme.
                                -this map is essentially a primitive array; that is, there are no actual keys
                                -each entry is just a block number, which can be stored in 2 bytes
                                -this mean that each block reserved for this map can point to 256 blocks containing 
                                actual key->value mappings where the key is the inode number (2 bytes) and the value
                                is its location on disk (2 bytes)
                                - each entry is thus 4 bytes, so we can fit 128 entries per block, giving a bound of
                                128*256 = 32768 inodes that can be mapped per block that we reserve for the inode map!
                        
                        3) The number of inodes that can be marked as free/unused in our bit vector. 
                                this may not actually be an issue, depending on if we actually use this structure; 
                                as discussed above (albeit maybe not comprehensibly) if we dynamically create inodes

                                in the case that we want to keep things simple and statically allocate inodes, then
                                we see that we need one bit for each inode. Each block contains 512*8 = 4096 bits. 
                                To hit the previous bound of 2^15, we would need to use a bit map taking up 8 whole blocks.
                                Hell No. That makes me feel sick. I won't do it. At the time of writing, I have decided that
                                I don't want to do this; if we end up anting to allocate inodes statically, then we will have 
                                to hold the status (free or in use) of each inode in one bit of the inode map. This would result in 
                                halving the number of inodes which can be accomodated in the system, and so I would prefer not to 
                                take that tradeoff. This means that inodes would need to be created dynamically as users create new 
                                files.

RECURSIVE UPDATE PROBLEM (ref http://pages.cs.wisc.edu/~remzi/OSTEP/file-lfs.pdf)
We cannot use my idea of having inode numbers being tied directly to their location, as this
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
    holds the same name-to-inode-number mapping. Thus, through indirection, LFS avoids the recursive update problem."

Preventing us from having sparse inodes:
    - We are going to be writing log changes to an in-memory segment
    - Just put all updated inodes in blocks together at the end of the segment before we write out to disk
    - The logistics of this might get a tad hairy, and we will need to be very careful with the math needed for us
    to ensure that inode pointers are correct
    - Updates to the inode map will also be needed here. The reserved blocks can be updated in place or overwritten 
    entirely by just re-formulating the content based on the in-memory inode map

When removing an inode, make sure to free up any indirect blocks allocated.

Note: we may need want to use unsigned shorts for into the inode map. 