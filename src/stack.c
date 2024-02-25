#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

/* Stack */
static Value pop_stack(Stack *stack);
static void  push_stack(Stack *stack, Value value);
static Value peek_stack(Stack *vm, int32_t distance);
static void  print_stack(Stack *vm);
static void  reset_stack(Stack *stack);

static bool stack_overflow(Stack *stack, int32_t index);

const StackAPI ant_stack = {
   .push = push_stack,
   .pop = pop_stack,
   .peek = peek_stack,
   .print = print_stack,
   .reset = reset_stack,
   .is_overflow = stack_overflow,
};

/* */

static void push_stack(Stack *stack, Value value) {
  int32_t stack_index = (int32_t)(stack->top - stack->slots);

  if (stack_index == OPTION_STACK_MAX) {
    fprintf(stderr, "Stack overflow\n");
    exit(11);
  }

  *(stack->top) = value;
  stack->top++;
}

/* */

static Value pop_stack(Stack *stack) {
  if (stack->top == stack->slots) {
    fprintf(stderr, "Stack underflow\n");
    exit(11);
  }

  stack->top--;
  return *(stack->top);
}

/* */

static Value peek_stack(Stack *stack, int32_t distance) {
  return stack->top[-1 - distance];
}

/* */

static void print_stack(Stack *stack) {
  printf("        ");
  for (Value *slot = stack->slots; slot < stack->top; slot++) {
    printf("[");
    ant_value.print(*slot, true);
    printf("]");
  }

  if (stack->top == stack->slots) {
    printf("[]");
  }

  printf("\n");
}

/* */

static bool stack_overflow(Stack *stack, int32_t stack_index) {
  return  stack->slots == stack->top || stack_index == OPTION_STACK_MAX ||
         stack->top < &stack->slots[stack_index];
}

/* */

static void reset_stack(Stack *stack) { stack->top = stack->slots; }

