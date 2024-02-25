#ifndef ANT_STACK_H
#define ANT_STACK_H

#include "common.h"
#include "value.h"
#include "config.h"

typedef struct {
   Value       slots[OPTION_STACK_MAX]; 
   Value*      top;    
} Stack;

typedef struct {
   void  (*reset)  (Stack* stack);
   void  (*push)  (Stack* stack, Value value);
   Value (*pop)   (Stack* stack);
   Value (*peek)  (Stack* stack, int32_t distance);
   void  (*print) (Stack* stack);
   bool  (*is_overflow) (Stack* stack, int32_t index);
}StackAPI;

const extern StackAPI ant_stack;

#endif
