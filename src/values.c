#include "values.h"
#include "memory.h"

#include <stdio.h>

static void init_value_array(ValueArray *array);
static void write_value_array(ValueArray *array, Value value);
static void free_value_array(ValueArray *array);
static void print_value(Value value);

static Value from_bool(bool value);
static Value from_number(double value);
static Value from_nil();
static bool to_bool(Value value);
static double to_number(Value value);

static bool is_bool(Value value);
static bool is_number(Value value);
static bool is_nil(Value value);

ValuesArrayAPI ant_values = {.init = init_value_array,
                             .write = write_value_array,
                             .free = free_value_array,
                             .print = print_value};

ValueAPI ant_value = {.from_bool = from_bool,
                      .from_number = from_number,
                      .from_nil = from_nil,
                      .to_bool = to_bool,
                      .to_number = to_number,
                      .is_bool = is_bool,
                      .is_number = is_number,
                      .is_nil = is_nil};

static void init_value_array(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

static void write_value_array(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int32_t old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values =
        GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

static void free_value_array(ValueArray *array) {
  if (!array)
    return;

  FREE_ARRAY(Value, array->values, array->capacity);
  init_value_array(array);
}

static Value from_bool(bool value) {
  return (Value){.type = VAL_BOOL, .as.boolean = value};
}

static Value from_number(double value) {
  return (Value){.type = VAL_NUMBER, .as.number = value};
}

static Value from_nil() { return (Value){.type = VAL_NUMBER, .as.number = 0}; }

static bool to_bool(Value value) {
  if (value.type != VAL_BOOL) {
    fprintf(stderr, "Value is not a boolean. Returning default: false\n");
    return false;
  }

  return value.as.boolean;
}

static double to_number(Value value) {
  if (value.type != VAL_NUMBER) {
    fprintf(stderr, "Value is not a number. Returning default: 0\n");
    return 0;
  }

  return value.as.number;
}

static bool is_bool(Value value) { return value.type == VAL_BOOL; }

static bool is_number(Value value) { return value.type == VAL_NUMBER; }

static bool is_nil(Value value) { return value.type == VAL_NIL; }

static void print_value(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf("%g", value.as.number);
    break;
  }
}
