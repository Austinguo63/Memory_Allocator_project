#include "interface.h"
#include "my_memory.h"

#include <stdio.h>
#include <assert.h>

// Interface implementation
// Implement APIs here...

void my_setup(enum malloc_type type, int mem_size, void *start_of_memory)
{   
    memory_size = mem_size;
    memory_offset = start_of_memory;
    allocator_type = type;
    // calculate the number of buddy sizes we need
    memory_size_2s_power = find_2s_power(mem_size);
    printf("mem_size_power = %d\n", memory_size_2s_power);

    min_chunk_2s_power = find_2s_power(MIN_MEM_CHUNK_SIZE);
    printf("min_chunk_size_power = %d\n", min_chunk_2s_power );

    num_buddy_levels = memory_size_2s_power - min_chunk_2s_power + 1;

    assert(sizeof(struct BuddyHeader) <= HEADER_SIZE);
    printf("Buddy header size %ld\n", sizeof(struct BuddyHeader));
    printf("System pointer size %ld\n", sizeof(size_t));

    init_buddy();

    printBuddyState();
    
}

void *my_malloc(int size)
{   
    switch (allocator_type){
        case MALLOC_BUDDY:
            buddy_malloc(size);
            break;
        case MALLOC_SLAB:
            slab_malloc(size);
            break;
        default:
            assert(false);
    }
    
}

void my_free(void *ptr)
{
    switch (allocator_type){
         case MALLOC_BUDDY:
            buddy_free(ptr);
            break;
        case MALLOC_SLAB:
            slab_free(ptr);
            break;
        default:
            assert(false);
    }
}
