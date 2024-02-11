#include "value_array.h"
#include "memory.h"

#include <stdio.h>

static void init_value_array(ValueArray *array);
static void init_value_array_with_nils(ValueArray *array);
static void write_value(ValueArray *array, Value value);
static bool write_value_at(ValueArray *array, Value value, int32_t index);
static void free_value_array(ValueArray *array);
static Value find_at(ValueArray *array, int32_t index);

static void write_nils(ValueArray *array, int32_t from, int32_t to);
ValuesArrayAPI ant_value_array = {
    .init = init_value_array,
    .init_nils = init_value_array_with_nils,
    .write = write_value,
    .write_at = write_value_at,
    .free = free_value_array,
    .at = find_at,
};

static void init_value_array(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
  array->has_nils = false;
}

static void init_value_array_with_nils(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
  array->has_nils = true;
}

static void write_value(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int32_t old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values =
        GROW_ARRAY(Value, array->values, old_capacity, array->capacity);

    if (array->has_nils) {
      write_nils(array, old_capacity, array->capacity);
    }
  }

  array->values[array->count] = value;
  array->count++;
}

// TODO: Make note about write_value_at and write_value not being used together
static bool write_value_at(ValueArray *array, Value value, int32_t index) {

  if (index < 0) {
    fprintf(stderr, "ValueArray: Index out of bounds\n");
    return false;
  }

  if (array->capacity <= index) {
    int32_t old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(index + 1);
    array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);

    if (array->has_nils) {
      write_nils(array, old_capacity, array->capacity);
    }
  }

  array->values[index] = value;

  // write_value_at and write_value not are supposed to be used together
  // but if they do, we need to update the count
  if (index >= array->count) {
    array->count = index + 1;
  }

  return true;
}


static Value find_at(ValueArray *array, int32_t index) {
  if (index < 0 || index >= array->capacity) {
    fprintf(stderr, "ValueArray: Index out of bounds\n");
    return ant_value.make_nil();
  }

  if(!array->has_nils){
     fprintf(stderr, "ValueArray: using .at method with No nils in the array.\n");
  }

  return array->values[index];
}

static void free_value_array(ValueArray *array) {
  if (!array)
    return;

  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}

static void write_nils(ValueArray *array, int32_t from, int32_t to) {
  for (int32_t i = from; i < to; i++) {
    array->values[i] = ant_value.make_nil();
  }
}
