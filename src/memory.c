#include "memory.h"
#include <stdlib.h>

/*
   old_size  	new_size	               Operation
   -------- 	--------	               -----------
   0	         Non‑zero	               Allocate new block.
   Non‑zero	   0                    	Free allocation.
   Non‑zero	   Smaller than old_size	Shrink existing allocation.
   Non‑zero  	Larger than old_size	   Grow existing allocation.
*/

void *reallocate(void *pointer, size_t old_size, size_t new_size) {
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
