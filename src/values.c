#include "values.h"
#include "memory.h"

#include <stdio.h>


void init_value_array(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_value_array(ValueArray *array, Value value){
   if(array->capacity < array->count +1){
      int32_t old_capacity = array->capacity;
      array->capacity = GROW_CAPACITY(old_capacity);
      array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
   }

   array->values[array->count] = value;
   array->count++;
}

void free_value_array(ValueArray *array){
   if(!array)
      return;

   FREE_ARRAY(Value, array->values, array->capacity);
   init_value_array(array);
}


void print_value(Value value){
   printf("%g\n", value);
}
