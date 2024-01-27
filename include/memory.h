#ifndef ANT_MEMORY_H
#define ANT_MEMORY_H

#include "common.h"
#define MIN_CHUNK_CAPACITY 8

#define GROW_CAPACITY(capacity) \
   ((capacity) < MIN_CHUNK_CAPACITY ? MIN_CHUNK_CAPACITY : (capacity) *2)

#define GROW_ARRAY(type, pointer, old_capacity, new_capacity) \
   (type*)reallocate(pointer, (sizeof(type)) *(old_capacity), (sizeof(type)) *(new_capacity))

// when passing 0 as new_size, it will free the memory
#define FREE_ARRAY(type, pointer, capacity) \
   reallocate(pointer, sizeof(type) * (capacity), 0)

void *reallocate(void *pointer, size_t old_size, size_t new_size);
#endif
