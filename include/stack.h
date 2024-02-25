#ifndef ANT_STACK_H
#define ANT_STACK_H

#include "common.h"
#include "value.h"
#include "config.h"

/* NOTE:
 * Stack operations are extremely performance sensitive so the have use of macros
 * to avoid function call overhead.
 */

typedef struct {
   Value       slots[OPTION_STACK_MAX]; 
   Value*      top;    
} Stack;

extern Stack stack;

#define STACK_PUSH(value) do { \
    if ((stack.top - stack.slots) >= OPTION_STACK_MAX) { \
        fprintf(stderr, "Stack overflow\n"); \
        exit(11); \
    } \
    *stack.top++ = (value); \
} while (0)

#define STACK_POP() ( \
    (stack.top == stack.slots) ? \
        (fprintf(stderr, "Stack underflow\n"), exit(11), stack.slots[0]) : \
        (*--stack.top) \
)


void print_stack(void);

#define STACK_PEEK(distance) (*(stack.top - 1 - (distance)))

#define STACK_RESET() (stack.top = stack.slots)

#define STACK_SET_TOP(top) (stack.top = (top))

#define STACK_TOP() (stack.top)

#define STACK_DECREMENT_TOP(by) (stack.top -= (by))

#define STACK_OVERFLOW(index) (stack.slots == stack.top || (index) == OPTION_STACK_MAX || stack.top < &stack.slots[(index)])

#define STACK_AT(index) (stack.slots[(index)])

#define STACK_PRINT() print_stack()
//const extern StackAPI ant_stack;

#endif
