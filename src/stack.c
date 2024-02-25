#include "stack.h"
#include <stdio.h>

Stack stack;

void print_stack(void) {
  printf("        ");
  for (Value *slot = stack.slots; slot < stack.top; slot++) {
    printf("[");
    ant_value.print(*slot, true);
    printf("]");
  }

  if (stack.top == stack.slots) {
    printf("[]");
  }

  printf("\n");
}
