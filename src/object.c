#include "object.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJECT(type, objectType)                                      \
  (type *)allocate_object(sizeof(type), objectType)

static ObjectType get_type(Value value);
static bool is_string(Value value);

static bool is_object_type(Value value, ObjectType type);
static void print_object(Value value);
static bool equals(Value a, Value b);

static Object *allocate_object(size_t size, ObjectType object_type);
static void free_object(Object *object);

ObjectAPI ant_object = {
    .type = get_type,
    .is_string = is_string,
    .print = print_object,
    .equals = equals,
    .allocate = allocate_object,
    .free = free_object,
};

/* */

static ObjectType get_type(Value value) {
  return ant_value.as_object(value)->type;
}

/* */

static bool is_string(Value value) { return is_object_type(value, OBJ_STRING); }

/* */

static bool is_object_type(Value value, ObjectType type) {
  return ant_value.is_object(value) && get_type(value) == type;
}

static Object *allocate_object(size_t size, ObjectType object_type) {
  Object *object = (Object *)ant_memory.realloc(NULL, 0, size);
  object->type = object_type;

  return ant_memory.add_object(object);
}

/* */

static void print_object(Value value) {
  switch (get_type(value)) {
  case OBJ_STRING:
    ant_string.print(value);
    break;

  default:
    break; // Unreachable
  }
}

/* */

static bool equals(Value a, Value b) {
  if (get_type(a) != get_type(b))
    return false;

  switch (get_type(a)) {
  case OBJ_STRING:
    return ant_string.equals(a, b);
    break;
  }
}


// FIX: Strings leave in a hash table now
static void free_object(Object *object) {
  switch (object->type) {
  case OBJ_STRING: {
    //ObjectString *string = (ObjectString *)object;
    //FREE_ARRAY(char, string->chars, string->length + 1);
    //FREE(ObjectString, string);
  }
  }
}
