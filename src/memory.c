#include "memory.h"
#include <stdlib.h>

GarbageCollection garbage = {.objects = NULL};

static void *reallocate(void *pointer, size_t old_size, size_t new_size);
static Object* add_object(Object *object);
static void    free_objects(void);

MemoryAPI ant_memory = {
    .add_object = add_object,
    .free_objects = free_objects,
    .realloc = reallocate,
};

/*
   old_size  	new_size	               Operation
   -------- 	--------	               -----------
   0	         Non‑zero	               Allocate new block.
   Non‑zero	   0                    	Free allocation.
   Non‑zero	   Smaller than old_size	Shrink existing allocation.
   Non‑zero  	Larger than old_size	   Grow existing allocation.
*/

static void *reallocate(void *pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *ptr = realloc(pointer, new_size);

  if (ptr == NULL) {
    exit(1);
  }

  return ptr;
}

#include <stdio.h>
static Object* add_object(Object *object) {
   /* add to the front  */

   object->next = garbage.objects;
   garbage.objects = object;
   return object;
}

static void free_objects(){
   Object *head = garbage.objects;


   while (head != NULL) {
      Object *next = head->next;
      ant_object.free(head);
      head = next;
   }

   garbage.objects = NULL;
}

