#ifndef ANT_VALUES_H
#define ANT_VALUES_H

#include "common.h"

typedef double Value;

typedef struct {
   int32_t capacity;
   int32_t count;
   Value *values;
}ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
void print_value(Value value);

#endif // ANT_VALUES_H
