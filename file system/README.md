# CSC 360 Assignment 4: Little Log Structured File System (LLFS)

## For the TAs

Hello and welcome. 

My disk implementation is pretty simple and intuitive. See the API below for details,
but it has a read fucntion and a write function, each of which uses one block. I
also added the ability for users to specify custom locations for these operations,
which makes the system more scalable because multiple disks can be used/mounted because 
of this.

The file system its self is in file.c. Were I to do it again I would have ignored
the instruction to make it all in one file, as it is very unweildy and messy at just 
under 1kloc.

In the readme I have a synopsis of the user API for the file system. Much of this 
is not yet fully functional, so I would encourage you to take a look at all of the
supporting framework functionality that would be used in implementations. For example,
for writing a file I have methods to find the blocks used for the file, data structures
to support checkpointing the written blocks, etc. 

If you would also like a structural overview, have a look into the file.h header file.
It is easier to parse than the source code file I think.

You will find a discussion of some of the design decisions and challenges I came 
up against (Yvonne wanted us to call it "[our] journey"...) down below in the 
README. There are two design decisions I would like to highlight. 
1. The choice to allow for 64 blocks to reside in the checkpoint buffer. This 
prevents me from doinglots and lots of io, but still allows for pretty consistent 
automated synching to disk meaning that little data will be lost. Users are also
able to make use of the flush_LLFS function to make sure that their changes are 
pervasive. This makes the system robust in the face of crashes. 
2. The other decision I would like to highlight is the use of a level of indirection
for the inode map, allowing us to have 4096 inodes (more than the maximum number
of non-empty files, given the disk size) as opposed to the 256 allowed if we only
used one byte for the inode id's. This was suggested in the spec as we are only supposed
to use 1 byte for the inode number in directory entries, but to get around this 
constraint I just stored the block key for the inode in these entries. (See below
for more info on how I structured my inodes).

For testing, I tried to write unit tests for everything I implemented, as I implemented it.
That said, there are a handful of functions that are not yet tested in file.c because either
they are simple and a dependency of others which were tested or because I didn't yet have time.

Unit tests for the disk controller are in disk/test_vdisk.c. 
Unit tests for the file system are in io/test_file.c. 

`make` will compile the file system library and the disk library as well as 
create 2 executables, `test_vdisk` and `test_file` which are the unit 
tests for the two libraries.

`make run_unittest` will do the above compilation and run both unit test suites.

Note: I verified that all 4 disk tests and the first 14/15 file tests should pass on
the ssh server.

I tried to strike a balance on the unit test being verbose and the output being easy to parse.
One in particular (file.c test 13) spits out a bunch of garbage due to repeated function calls
but this output is needed to make the other tests make sense. Sorry about that!

I have also included the apps/test0X files detailed in the spec. ```test01.c```
shows how blocks can be written to and read from the disk. A child process is
created to write some content to the disk, and the parent reads it back and 
validates the result once the child is done. You can compile this with `make test01`. 
`test02c` shows how a new disk can be created (with `vdisk_write`) and mounted 
(with `init_LLFS`). `make test02` will compile it.

The shell script `script.sh` (very imaginative title) can be run with `bash script.sh`.
This will build the libraries, run the unit tests, and run the apps/tests as well.
Output from the bash script is marked clearly in an effort to make the result more 
readible, as it produces quite a bit of stuff.

## APIs and spec notes

### <u>vDisk</u>
* We use a file called "vdisk" as a virtual disk for this assignment
* Deleting the vdisk lets us start from scratch 
* vdisk is usually assumed to be in the current directory, but read/write have been parameterized to take a vdisk FILE stream pointer as well
* blocks are 512 bytes
* there are 4096 blocks on the disk (indexed from 0)

The ```disk/``` directory:  
* ```disk/vdisk.c``` is the disk controller.
* ```disk/vdisk.h``` is a header for the disk controller.
* ```disk/test_vdisk.c``` holds unit tests for the disk


### ```/disk/vdisk.c```
```bool vdisk_read(int block_number, void *buffer, FILE *alt_disk)```
    
    Lets the file system read one block from the specified vdisk into a buffer provided. If no disk file is given, the vdisk file in the current directory is used. 

```bool vdisk_write(int block_number, void *content, int offset, int content_length, FILE *alt_disk)```

    Lets the file system write one block to the disk from the content buffer. Content is truncated to prevent overflowing the one block limit. Offsetting content from the start of the specified block is supported. If no disk file is given, the vdisk file in the current directory is used.


### __LLFS__

The full LLFS API is too messy/big to write out here and have it be meaningful.
The user-accesssible functions are:

```void init_LLFS();```
Format and mount the disk [implemented and tested]

```void flush_LLFS();```
Flush changes from the checkpoint buffer to disk [implemented and tested]

```void defrag_LLFS();```
Move chunks of disk back to fill holes and make more space for the log [unmplemented]

To implement this my idea is to iterate from the end of the disk. Each time I hit an inode block,
move all associated data blocks back in order to fill holes (I would have a counter indicating where the
next gap is, starting from the beginning). This wouldn't be too resource intensive and 
would certainly allow much more room for the log to grow.

```void terminate_LLFS();```
Flush the disk and free used memory [implemented and tested]

```inode_t *create_file(char *filename, char *path_to_parent_dir);```
Create a new, empty, non-directory file in the parent given [implemeneted but not thoroughly tested]

```inode_t *mkdir(char * dirname, char *path_to_parent_dir); ```
Create a new empty directory as a subdirectory of the parent given [unimplemented]

To implement this I would be able to reuse a lot of the code from create_file, but making 
sure to set the dir flag for the inode

```int write(void* content, int content_length, int offset, char *filename); ```
Overwrite/add to the file at a given offset. [unimplemented]

To implement, this would require the use of create_file if no such file currently 
exists, and then just adding data blocks to the inode corresponding to that file.

```int append(char *content, int content_length, char *filename); ```
Same as write, but with offset equal to file length (i.e. no overwriting can happen). [unimplemented]

To implement, this would require the use of create_file if no such file currently 
exists, and then just adding data blocks to the inode corresponding to that file.

```char **get_dir_contents(char *dirname);```
Returns a list of the filenames in a given directory

```int read_file(void *buffer, int buffer_size, char *filename); ```
Copies content from the file's data blocks into the buffer [implemented but untested]

```bool rm(char *filename);```
Remove the specified file or directory [unimplemented]

To implement this would be fairly simple, but should come after the above operations 
so that they can be used in testing. The just involves verifying that if it is a directory
that it is empty, removing the pointer to it from its parent inode (id is stored in its inode)
and freeing the associated data and inode blocks.

## My "Journey" as Yvonne likes to say

### <u>The disk</u>
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


### <u>Starting the File System: Free List & Inodes</u>

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

We will use one of the remaining bits in the 2 byte inode number to indicate 
whether the corresponding file is a directory.

We will use our last 2 bytes inside each inode for the parent directory's inode
number. This will give us an easy way to ensure that we are 100% accurate when
looking up some file within a directory. The case in question is if two inodes
in the same block correspond to files with the same name with different parent
directories. This way we won't choose the wrong one.

### <u>Implementing Inode Stuff</u>

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
    were found while probing for sanity checks, now for the actual testing!

    The init function has lots of responsibilities so I broke the test up 
    into 3 parts. All three now pass (after some silly fixes).


### <u>Starting on LLFS Functionality</u>

We can now move on to adding support for writing to files. For this we will need
to come up with an API for our file system (read and write should be sufficient 
for now). We also need a checkpoint buffer in which to store changes. At its 
heart, this should be an array of blocks, in the order that we would like to 
write them. We can choose to write this buffer to disk when it gets full, but we
need to be careful of only sending part of some transaction to disk. A better 
way to go would be to check before starting each write whether it will cause the
buffer to fill/overflow. If so, write the current content (and update the inode 
map, inode free list, and block free list), clear the buffer and continue. 
Another thing we need to watch out for is single transactions which can cause 
the buffer to overflow (i.e. writing a file larger than our buffer). In this 
case, the best way to go seems to be to split it up into separate transactions.

    The first thing is the API. For now, the only functionality I want to implement
    is reading and writing non-directory files to root.

    Once that's working we need to add in the ability to make subdirectories of 
    root.

    From there we should add in the ability to delete files/dirs from root

    Next up is porting the 3 functions above so that they can take place 
    anywhere in the system, i.e. so that we can read any file in any directory,
    write to any directory, and delete from any leaf node (that is, any file or
    any empty directory).  

Design Question: Should we treat files and directories differently for the above 3 options?
    We can't do this for write, since users should not be able to just write to 
    the data blocks of a directory! Thus we'll go with mkdir() and write(). They
    can probably share some code under the hood though.

    For delete (rm?) this seems reasonable to share a sinlge function, 
    especially since we're allowing directories to be deleted only if they are 
    empty. 

    Reading a file should just yield a string of the bytes in the file, but 
    reading a directory should involve some formatting of the results. We don't 
    want to be giving the user a list of inode numbers along with the filename 
    etc.Also, they might also want some of the metadata like file size for each 
    item. We will give just a char** containing all file names and add other
    functions to allow users to access metadata via that file name. This 
    prevents them from needing to deal with erroneous structs or filter through
    strings of bytes representing multiple data types.
    We will separate file and directory reading functions, but we can certainly 
    re-use some of the code.
                
In order to move forward with implementing first-round versions of these 
functions, we will need a checkpointing system. 

I wrapped a block buffer in a struct where we can keep track of how full it is 
and any other metadata we need. For now, the buffer size is set to accomodate
64 blocks. This seems small enough to be reasonable from a resource point of 
view but large enough that we won't need to go to disk every time something is 
modified. I also added helper functions to add blocks to it, retrieve stuff from it,
set it up, and destroy it.

[TODO]
Here is the API I've defined for now. We'll definitely need to add some more 
functions as we go. We need tests for these too!

    void init_LLFS()        DONE
    void terminate_LLFS()   Can wait until the end, mainly just uses flush
    void flush_LLFS()       DONE
    void defrag_LLFS()      

    inode_t *create_file(char *filename, char *path_to_parent_dir) 
    inode_t *mkdir(char * dirname, char *path_to_parent_dir)
    int write(void* content, int content_length, int offset, char *filename)
    int append(char *content, int content_length, char *filename);

    char **get_dir_contents(char *dirname)
    int read_file(void *buffer, int buffer_size, char *filename); 

    bool rm(char *filename);

    inode_t* find_dir(char* dirpath);

It is also worth noting here that we need to make sure that root cannot be rm'd. 

In the process of writing read and write, I noticed that we will probably need a
flag to tell us whether a given file has unsynched changes in the buffer. This is
because if the file is not cached (which it probably won't be because I'm unlikely 
to have time to add a file cache) we need to be able to tell if its disk version is up to 
date when it is read. Otherwise we would grab the disk version without the 
recent changes appearing in the buffer.
    
    Option 1: Keep track of whether a file is changed by adding a flag to its 
              inode that tells us whether it is 'dirty'. A problem with this 
              approach is that it may require us to go to disk for the check in 
              the case that the relevant inode is not cached. Because of this, 
              it makes sense to go with the second option.

    Option 2: Store the data of which files have been changed in the checkpoint 
              buffer. When reading, if a file has been changed, flush to disk 
              before perforing read.

              To do this we could group the buffer into transactions, and 
              concatenate their payloads when flushing to disk.

              Alternatively, we could add another data structure (an array, 
              perhaps) of inode number of files which have been changed. 
              If we did this with a bit vector we could have very fast lookup 
              time at the cost of 512 bytes of space. This seems like a good 
              tradeoff, so we'll take it! 

              Added a dirty_inode_list to the checkpoint buffer struct to let us
              track this.

Added this functionality to get_block() etc. and tested that it works! Yay!


its getting pretty tough to track where we're at on disk and in memory just using
the next_available fields in the bitvectors. It might be better to add two globals
to tell us where we're at in each. i.e. where the end of log is on disk and where 
it is in memory. Right now, we just have free_block_list->next_available which gives
the address of the next available block given the state in memory. We don't have a 
direct index for the spot immediately after the log on disk. We can find it by taking
the value above and subtracting the number of items in the checkpoint buffer though.

Added filename validation to the create_file function to check that it not malformed
and does not already exist in the parent directory specified. It now relies on a few
other functions including get_dir_entries which is unimplmented (and obviously untested).

Added loads of helper functions like get_block, get_blocks, get_dir_entries, etc. to help with 
reading and writing tasks. These have (almost) all been tested!

I changed the get_inode_block function to grab from the checkpoint buffer.

[TODO] Another thing to we need to figure out is the inode cache that we're 
going to use. It feels easiest to hold an array of arrays of inode_t pointers,
each sub-array corresponding to one of the inode blocks. I'm also going to need
a policy on how to manage it when it gets full. One option would be to have the 
outer array act as a queue (of sorts), and when it gets full we delete the 
oldest block of inodes to make room for another one. This would let us do either
a FIFO policy, or a LRU policy if we allow shuffling the queue when we use 
something already in it. This seems like a better way to go, but it is a little
more complicated.

Its looking like this isn't actually going to get done; I have a final on the 11th that I
need to put time in to study for, so this is going to be as far as I get. As above,
what I would have liked to do would be the following:
- Implement read and write as described above (and in more detail in file.c) to read and write only to root. This would require a couple other helper functions but almost all of the framework functionality to make it happen is there (which hurts! :/)
- Implement find_dir to parse through the directory tree and return the inode of the directory passed in. From here, we would be able to support all our operations in directories other than root.

### Misc Notes

When removing an inode, make sure to free up any indirect blocks allocated.

Checkpointing:
    - need a segment buffer to be sent to disk when full (this is a collection of
    new stuff to add to the end of the log) 
    - Need a way to ensure this is written before a calling process terminates so we don't lose progress
            - One way to do this is to put the onus on the user to perform a manual
              checkpoint before ending a program which uses this library 
              (essentially like closing a file system) 
    DONE: Flush/terminate take care of this.
We need to make sure that we're populating our data structures in memory properly
from disk before users are able to actually use the file system. One simple way to do
this is to define a user API which is a subset of the functions written here (essentially
everything that would be made public were we doing this in Java) and in each of these fuctions
check whether we have done this population (if not, do it then).
Alternatively, we could ask that the user call a setup function before using the library. This
falls in line nicely with a cleanup-style function mentioned in the checkpointing note above. 
    OPTED FOR THE SECOND OPTION (INIT, FLUSH, TERMINATE)