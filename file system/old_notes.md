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