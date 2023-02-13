#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include "interface.h"

// Declare your own data structures and functions here...
struct BuddyHeader{
    // the address of the next buddy block of the same size
    // in units of MIN_MEM_CHUNK_SIZE
    // relative to mem_offset
    int32_t next_chunk;
    uint8_t size_2s_power;
    uint8_t is_right;
    uint8_t allocated;
};

struct BuddyLocation{
    int size_index;
    int32_t this_chunk;
    int32_t last_chunk;
};


struct SlabMeta{
    int size;
    int allocation[N_OBJS_PER_SLAB / (sizeof(int) * 8)];
    void* start_address;
    struct SlabMeta *next;
};

int find_2s_power(int size);
struct BuddyHeader *next_buddy_to_address(int32_t next_buddy);
int32_t address_to_next_buddy(void *next_buddy_to_address);

void init_buddy();

struct BuddyLocation find_free_buddy_chunk_of_size(int index);
struct BuddyLocation find_place_to_insert(int index, uint32_t offset_of_chunk_to_insert);
struct BuddyLocation buddy_remove_chunk(struct BuddyLocation location);
struct BuddyLocation split_chunk(struct BuddyLocation location);

struct BuddyLocation merge_chunk(struct BuddyLocation location);
bool buddy_merge_chunks_of_size(int index);
void buddy_merge_scan(int start_index);

void* buddy_malloc(int size);
void buddy_free(void *ptr);

bool allocate_new_slab(int size);
void destroy_empty_slab(struct SlabMeta *slab_to_remove);
void* slab_malloc(int size);
void slab_free(void *ptr);


void printBuddyState();
void printBuddyChunkOfSize(int index);
void printSize(size_t size);

extern void *memory_offset;
extern enum malloc_type allocator_type;
extern int memory_size;
extern int memory_size_2s_power;
extern int num_buddy_levels;
extern int min_chunk_2s_power;
extern int32_t *buddy_table;
extern struct SlabMeta *slab_meta;



#endif
