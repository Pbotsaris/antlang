#ifndef ANT_MEMORY_H
#define ANT_MEMORY_H

#include "common.h"
#include "object.h"

#define MIN_CHUNK_CAPACITY 8

typedef struct {
   Object* objects;
}GarbageCollection;

typedef struct {
   void*   (*realloc)(void *pointer, size_t old_size, size_t new_size);
   Object* (*add_object)(Object *object);
   void    (*free_objects)(void);
}MemoryAPI;

extern MemoryAPI ant_memory;

#define GROW_CAPACITY(capacity) \
   ((capacity) < MIN_CHUNK_CAPACITY ? MIN_CHUNK_CAPACITY : (capacity) *2)

#define GROW_ARRAY(type, pointer, old_capacity, new_capacity) \
   (type*)ant_memory.realloc(pointer, (sizeof(type)) *(old_capacity), (sizeof(type)) *(new_capacity))

// when passing 0 as new_size, it will free the memory
#define FREE_ARRAY(type, pointer, capacity) \
   ant_memory.realloc(pointer, sizeof(type) * (capacity), 0)

#define ALLOCATE(type, count) \
   (type*)ant_memory.realloc(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) ant_memory.realloc(pointer, sizeof(type), 0)

//void *reallocate(void *pointer, size_t old_size, size_t new_size);

#endif
