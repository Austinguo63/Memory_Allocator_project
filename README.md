# Memory-Allocator-CPMSC473-2022

## Author
Jinyu Liu (JZL6359@psu.edu)

Hongyu Guo (HBG5147@psu.edu)

## Introduction
This is a project that focus on implementing two memory allocation schemes on a single CPU system, Buddy and Slab.

Buddy always allocate memory size that are powers of two.
Allowing for easy de-fragmentation of memory after frees.
But Buddy allocation will incurs large amount of external 
fragmentation due to the power of 2 limitation and is not
very efficient at allocating large number of objects.
Slab allocation addresses this problem, by exploiting the fact
that memory allocations mostly happen with only a few different
sizes. Slab allocates space for many objects of the same size
at once, cutting down on the number of external fragmentation
incurred in Buddy, thus improving memory efficiency for typical
use cases.
 

## Main Data Structures

### BuddyHeader
The header stores many book-keeping values for the Buddy 
allocator. Since this need to fit in to the 8 bytes allocated
for the header, it need to be space efficient.
They are organized in linked lists using the
`next_chunk` field. For the sake of efficiency, this is
represented as an offset from the start of memory in units
of `MIN_MEM_CHUNK_SIZE` which is 512 in this project.
Since the way Buddy works ensures that all chunks must start
and end at `MIN_MEM_CHUNK_SIZE` boundaries.
Negative values are defined to be invalid and an analog to 
`NULL`. This allows a signed 32-bit integer to be used to 
address up to 1TiB of memory. Three more 8-bit values are
stored in the header as well. `size_2s_power` stores the size
of the block as a power of 2. `is_right` is a boolean value to
denote if the chunk is higher or to the right of its potential
merge partner. This is used for merging. `allocated` shows if
the current chunk is allocated or not.

Overall the BuddyHeaders are arranged in an array of 
linked lists. Where each linked list contains all chunks of a 
certain size, free or allocated.

### SlabMeta
This data structure holds metadata for each slab. `size` holds
the size of the slots in this slab. `allocation` is a bit-field
representing if each slot is allocated or not. `start_address`
is a pointer to the start of the slab. And `next` is a pointer
to the next SlabMeta. Each slot in the slab contains a header
that is a pointer to that slab's SlabMeta.

The SlabMeta structures are arranged in a linked list.

## Theory of Operation

### Buddy Malloc

Buddy malloc starts by finding the minimum chunk size that can
fit the requested memory and the header. It scans the 
linked-list for that size trying to find a free chunk.
If none are available it try the next largest chunk size, this
repeats until a free chunk is found or none is available and
and the allocator is force to return an error. Once a free
chunk is found, it is split, multiple times if necessary, 
until a chunk of the right size is created. That chunk is 
marked as allocated and the address after the header is 
returned.
 
### Buddy Free

Buddy free first subtracts 8 from the given address to get to
the header. The header is marked as being free. Then the 
program scans for chunks to merge starting at the size of the
freed chunk. This ensures all chunks that can be merged are 
merged chunks are merged.

### Slab Malloc

It scans the linked-list of current slabs to find one with the 
right size and a free slot. If none are found it allocates
memory for one using `buddy_malloc`. Then it scans the slab
for the first available slot, marks it as allocated, and 
returns the address after the header for that slot.

### Slab Free
It subtracts 8 from the given address to get to the pointer to
the slab metadata. It marks the slot as freed. Then it checks
to see if the slab is empty. If it is, then the slab is 
destroyed by removing it from the linked list and `buddy_free` its memory.
















## Implementing Detial

### Rules 
1. The program runs on a single-threaded, max memory size of 8x1024x1024 bytes.  

2.  Buddy Allocation have mininum chunk size of 512 bytes. 

3.  Number of objects per slab is fixed in this project, which is 64 per slab. 
   


### The find_2s_power Function

It is the job of the find_2s_power function to find out the 2s power
of allocation size. Since system is base on 2, find 2s power of allocation size will
give us idea of how to give chunks. It will take size as input and
output power.

### The my_setup Function 
This function is use to set up all the required information for malloc, most importantly
find out how many levels Buddy Allocation will have . By find out
the size need to allocat (in 2s power) -  mininum chunk in 2s power + 1.  

### The init_buddy Function
This function get allocatd memory for inital. First malloc number of memory, then use 
memset to init all byte to -1. Follow up create inital chunk at start of memory given
in my_setup. Set those chunk to inital.

### The BuddyLocation
1.  find_free_buddy_chunk_of_size: use while loop to checck if a chunk is free or not. 
2.  find_place_to_insert:  useing while loop to find current chunk location is 
        infront or behind the one need insert, and insert it in right place. 
3.  body_remove_chunk: remove the chunk.
4.  split_chunk : when called, need chunk to be free and at least 2^10 large. Then 
        remove this chunk and split it into two chunks as half size of the original.
5.  merge_chunk: by remove both chunks from linked-list , then we rise 2s power by 1
        since chunk size is changed. Use find_place_to_insert to find right place to 
        insert the merged whole chunk.



### The buddy_malloc Function
 This function take size as input, and find 2s power of this (size+8) - 9. Substract 9 
 is because 2^9 is our mininum chunk size 512. If this desired_size_index is negative, that means 
 we need to allocate mininum size 512. Then search upwards for a free chunk, after find the
 chunk, split this chunk into right size. Lastly move pointer allocated to this position 
 and mark this chunk as allocatd. 




### The buddy_free Function
This is function take a pointer as input, and free from this address - 8.
it also use buddy_merge_scan to scan and merge eligible chunks.




 











