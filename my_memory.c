#include "my_memory.h"
#include <assert.h>
#include <stdlib.h>

// Memory allocator implementation
// Implement all other functions here...

int find_2s_power(int size){
    int power = 0;
    for (int size_temp = size - 1; size_temp != 0; size_temp >>= 1){
        power ++;
    }
    return power;
}

struct BuddyHeader *next_buddy_to_address(int32_t next_buddy){
    assert(next_buddy >= 0);
    return memory_offset + (next_buddy * MIN_MEM_CHUNK_SIZE);
}

int32_t address_to_next_buddy(void *next_buddy_to_address){
    return ((size_t)(next_buddy_to_address - memory_offset)) / MIN_MEM_CHUNK_SIZE;
}

void init_buddy(){
    // allocate memory, I guess we are not ever freeing it
    // just let the OS clean up the mess.
    buddy_table = malloc(sizeof(int32_t) * num_buddy_levels);
    // initialize all to negative 1
    memset(buddy_table, -1, sizeof(int32_t) * num_buddy_levels);

    // create the inital chunk
    // at the start of memory
    struct BuddyHeader *inital_chunk = memory_offset;
    inital_chunk -> next_chunk = -1; // no next block
    inital_chunk -> is_right = false;
    inital_chunk -> allocated = false;
    inital_chunk -> size_2s_power = memory_size_2s_power; //contains entire memory

    buddy_table[inital_chunk->size_2s_power - min_chunk_2s_power] = 0; // point the buddy_table entry to the inital_chunk
}

struct BuddyLocation find_free_buddy_chunk_of_size(int index){
    struct BuddyLocation location;
    location.this_chunk = buddy_table[index];
    location.last_chunk = -1;
    location.size_index = index;
    while(location.this_chunk >= 0){
        struct BuddyHeader *current_header = next_buddy_to_address(location.this_chunk);
        if (!current_header->allocated){
            break;
        }
        location.last_chunk = location.this_chunk;
        location.this_chunk = current_header->next_chunk;
    }
    return location;
}

struct BuddyLocation find_place_to_insert(int index, uint32_t offset_of_chunk_to_insert){
    struct BuddyLocation location;
    location.this_chunk = buddy_table[index];
    location.last_chunk = -1;
    location.size_index = index;
    while(location.this_chunk >= 0){
        struct BuddyHeader *current_header = next_buddy_to_address(location.this_chunk);
        if (location.this_chunk > offset_of_chunk_to_insert){
            break;
        }
        location.last_chunk = location.this_chunk;
        location.this_chunk = current_header->next_chunk;
    }
    return location;
}

struct BuddyLocation buddy_remove_chunk(struct BuddyLocation location){
    struct BuddyHeader *current_header = next_buddy_to_address(location.this_chunk);

    if(location.last_chunk < 0) {
        buddy_table[location.size_index] = current_header->next_chunk;
    } else {
        next_buddy_to_address(location.last_chunk)->next_chunk = current_header->next_chunk;
    }

    location.this_chunk = current_header->next_chunk;
    return location;
}

struct BuddyLocation split_chunk(struct BuddyLocation location) {
    int chunk_to_split_offset = location.this_chunk;
    struct BuddyHeader *chunk_to_split = next_buddy_to_address(chunk_to_split_offset);

    // make sure the chunk is free
    assert(!chunk_to_split->allocated);
    // make sure we are not trying to split a min-sized chunk
    assert(chunk_to_split->size_2s_power > min_chunk_2s_power);

    // removes it from the linked list for its oroginal size
    buddy_remove_chunk(location);

    // do the spliting
    int new_chunk_offset = chunk_to_split_offset + (1 << (chunk_to_split->size_2s_power - min_chunk_2s_power - 1));
    struct BuddyHeader *new_chunk = next_buddy_to_address(new_chunk_offset);

    chunk_to_split->size_2s_power --;
    chunk_to_split->is_right = false;
    chunk_to_split->next_chunk = new_chunk_offset;

    new_chunk->allocated = false;
    new_chunk->is_right = true;
    new_chunk->size_2s_power = chunk_to_split->size_2s_power;

    // do the insert
    struct BuddyLocation insert_location = find_place_to_insert(location.size_index - 1, chunk_to_split_offset);
    if(insert_location.last_chunk < 0) {
        buddy_table[location.size_index - 1] = chunk_to_split_offset;
    } else {
        next_buddy_to_address(insert_location.last_chunk)->next_chunk = chunk_to_split_offset;
    }

    new_chunk->next_chunk = insert_location.this_chunk;

    location.last_chunk = insert_location.last_chunk;
    location.size_index--;

    return location;
}

struct BuddyLocation merge_chunk(struct BuddyLocation location){
    int32_t left_chunk_offset = location.this_chunk;
    struct BuddyHeader *left_chunk = next_buddy_to_address(left_chunk_offset);
    // remove both chunks from the linked list of its original size
    buddy_remove_chunk(buddy_remove_chunk(location));

    left_chunk->size_2s_power++;

    // find a place to insert the bigger chunk
    struct BuddyLocation insert_location = find_place_to_insert(location.size_index + 1, left_chunk_offset);
     if(insert_location.last_chunk < 0) {
        buddy_table[location.size_index + 1] = left_chunk_offset;
    } else {
        next_buddy_to_address(insert_location.last_chunk)->next_chunk = left_chunk_offset;
    }

    left_chunk -> next_chunk = insert_location.this_chunk;

    location.last_chunk = insert_location.last_chunk;
    location.size_index ++;

    // determine if the new block is left or right
    left_chunk->is_right = (left_chunk_offset >> location.size_index & 1) != 0;

    return location;
}

bool buddy_merge_chunks_of_size(int index) {
    struct BuddyLocation location;
    location.this_chunk = buddy_table[index];
    location.last_chunk = -1;
    location.size_index = index;
    while(location.this_chunk >= 0){
        struct BuddyHeader *current_header = next_buddy_to_address(location.this_chunk);
        if (!current_header->is_right && !current_header->allocated){
            // only check for free left chunks
            // check if the corrsponding right chunk is there 
            if (current_header->next_chunk - location.this_chunk == (1 << index)){
                struct BuddyHeader *right_header = next_buddy_to_address(current_header->next_chunk);
                // check if the right chunk is free too
                if (!right_header->allocated){
                    // merge
                    merge_chunk(location);
                    return true;
                }
            }
        }
        location.last_chunk = location.this_chunk;
        location.this_chunk = current_header->next_chunk;
    }
    return false;
}

void buddy_merge_scan(int start_index){
    // only scan upwards.
    for (int i = start_index; i < num_buddy_levels - 1; i++){
        // printf("%d\n", i);
        if (!buddy_merge_chunks_of_size(i)){
            break;
        }
    }
}

void* buddy_malloc(int size){
    printf("Buddy Malloc Request for %d bytes\n", size);
    int desired_size_index = find_2s_power(size + HEADER_SIZE) - min_chunk_2s_power;

    if (desired_size_index < 0) {
        // allocate the minimum size if smaller than min_chunk_size
        desired_size_index = 0;
    } else if (desired_size_index >= num_buddy_levels) {
        printf("Requested Size Too Large\n");
        return (void *) -1;
    }

    printf("Size index is %d\n", desired_size_index);

    int size_index;

    struct BuddyLocation free_chunk;
    // search upwards for a free chunk
    for (size_index = desired_size_index; size_index < num_buddy_levels; size_index++) {
        free_chunk = find_free_buddy_chunk_of_size(size_index);
        if (free_chunk.this_chunk >= 0) {
            break;
        }
    }

    if(size_index >= num_buddy_levels){
        printf("Unable to find free chunk\n");
        return (void*) -1;
    }

    // split the chunk down the the correct size
    for (size_index = free_chunk.size_index; size_index > desired_size_index; size_index--){
        free_chunk = split_chunk(free_chunk);
    }
    
    struct BuddyHeader *allocated_chunk = next_buddy_to_address(free_chunk.this_chunk);
    allocated_chunk->allocated = true; // mark the chunk as allocated

    printBuddyState();

    return ((void*)allocated_chunk) + HEADER_SIZE;

}

void buddy_free(void *ptr){
    // get the header
    struct BuddyHeader *header = ptr - HEADER_SIZE;
    header->allocated = false;
    printf("Buddy Free of %d, size ", address_to_next_buddy(header));
    printSize(1 << header->size_2s_power);
    printf("\n");
    // scan and merge eligible chunks.
    buddy_merge_scan(header->size_2s_power - min_chunk_2s_power);
    printBuddyState();
}

void printSize(size_t size){
    static char* prefixes = " kMGTPE";
    int prefix_index;
    for(prefix_index = 0; prefix_index < 7 && size >= 1024; prefix_index++){
        size /= 1024;
    }

    if(prefix_index != 0) {
        printf("%3ld%ciB", size, prefixes[prefix_index]);
    } else {
        printf("%5ldB", size);
    }
    
}

void printBuddyChunkOfSize(int index){
    int32_t current_offset = buddy_table[index];
    int32_t last_offset = -1;
    size_t chunk_size = ((size_t)1) << (min_chunk_2s_power + index);
    printf("| ");
    printSize(chunk_size);
    printf(" |");
    int count = 0;
    while(current_offset >= 0 && count < 10) {
        if (current_offset - last_offset > (1 << (index))){
            printf(" -> ");
        }

        struct BuddyHeader *current_header = next_buddy_to_address(current_offset);
        printf("| %d : ", current_offset);
        if (current_header->allocated) {
            printf("used");
        } else {
            printf("free");
        }
        printf(" |");

        last_offset = current_offset;
        current_offset = current_header->next_chunk;
        count ++;
    }
    if (current_offset >= 0 && count >= 16){
        printf("...");
    }
    printf("\n");
}

void printBuddyState(){
#ifdef PRINT
    for (int i = 0; i < num_buddy_levels; i++) {
        printBuddyChunkOfSize(i);
    }
#endif
}

void* slab_malloc(int size){
    struct SlabMeta *current_slab = slab_meta;
    bool found = false;
    while (current_slab != NULL && !found) {
        if (current_slab->size == size){
            // check if there is a free slot in the slab
            for(int i = 0; i < (N_OBJS_PER_SLAB / (sizeof(int) * 8)); i++){
                if (current_slab->allocation[i] != -1){
                    found = true;
                    break;
                }
            }
        }

        if (!found){
            current_slab = current_slab->next;
        }
    }

    if (current_slab == NULL){
        if (!allocate_new_slab(size)){
            // failed to allocate new slab.
            return (void*)(-1);
        }
        current_slab = slab_meta;
    }

    // find the first open slot in the chosen slab
    int slot_index = -1;
    for(int i = 0; i < (N_OBJS_PER_SLAB / (sizeof(int) * 8)); i++){
        if (current_slab->allocation[i] != -1){
            for (int j = 0; j < sizeof(int) * 8; j++) {
                if (!(current_slab->allocation[i] & (1 << j))){
                    current_slab->allocation[i] |= (1 << j);
                    slot_index = i * sizeof(int) * 8 + j;
                    break;
                }
            }
            if (slot_index >= 0) {
                break;
            }
        }
    }

    assert(slot_index != -1);

    // set header
    void* slot_address = current_slab->start_address + slot_index * (current_slab->size + HEADER_SIZE);
    *((struct SlabMeta **) slot_address) = current_slab;

    return slot_address + HEADER_SIZE;

}

bool allocate_new_slab(int size){
    size_t slab_size = ((size_t) size + HEADER_SIZE) * N_OBJS_PER_SLAB;
    printf("Allocating new slab of total_size %ld\n", slab_size);
    void* slab_address = buddy_malloc(slab_size);
    if ((size_t)slab_address == -1) {
        return false;
    }

    //create new entry for slab meta
    struct SlabMeta *new_slab_meta = malloc(sizeof(struct SlabMeta));
    
    new_slab_meta->start_address = slab_address;
    new_slab_meta->size = size;
    // set all blocks to free
    memset(&new_slab_meta->allocation, 0, N_OBJS_PER_SLAB / 8);

    // insert slab at the start of the linked list.
    new_slab_meta->next = slab_meta;
    slab_meta = new_slab_meta;

    return true;
}

void destroy_empty_slab(struct SlabMeta *slab_to_remove){
    struct SlabMeta *current_slab = slab_meta;
    struct SlabMeta *last_slab = NULL;
    while (current_slab != NULL){
        if (current_slab == slab_to_remove){
            // remove the slab
            if (last_slab != NULL){
                last_slab->next = current_slab->next;
            } else {
                slab_meta = current_slab->next;
            }
            buddy_free(slab_to_remove->start_address);
            free(slab_to_remove);
            break;
        } 
        last_slab = current_slab;
        current_slab = current_slab->next;
    }
}

void slab_free(void *ptr){
    void* slot_address = ptr - HEADER_SIZE;
    struct SlabMeta *current_slab = *((struct SlabMeta **)slot_address);
    int slot_index = (slot_address - current_slab->start_address) / (current_slab->size + HEADER_SIZE);
    int allocation_word_index = slot_index / (sizeof(int) * 8);
    int allocation_bit_index = slot_index % (sizeof(int) * 8);
    current_slab->allocation[allocation_word_index] &= (~(1 << allocation_bit_index));

    bool isEmpty = true;
    for (int i = 0; i < (N_OBJS_PER_SLAB / (sizeof(int) * 8)); i++){
        isEmpty = isEmpty && (current_slab->allocation[i] == 0);
    }

    if (isEmpty){
        destroy_empty_slab(current_slab);
    }
}

int memory_size;
enum malloc_type allocator_type;
int num_buddy_levels;
int memory_size_2s_power;
int min_chunk_2s_power;
void *memory_offset;
int32_t *buddy_table;
struct SlabMeta *slab_meta;










