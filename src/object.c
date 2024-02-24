#include "object.h"
#include "memory.h"
#include "functions.h"
#include <stdio.h>
#include <string.h>

#define ALLOCATE_OBJECT(type, objectType)                                      \
  (type *)allocate_object(sizeof(type), objectType)

static ObjectType get_type(Value value);
static bool is_string(Value value);
static bool is_function(Value value);

static bool is_object_type(Value value, ObjectType type);
static int32_t print_object(Value value, bool debug);

static Object *allocate_object(size_t size, ObjectType object_type);
static void free_object(Object *object);

ObjectAPI ant_object = {
    .type = get_type,
    .is_string = is_string,
    .is_function = is_function,
    .print = print_object,
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

static bool is_function(Value value) { return is_object_type(value, OBJ_FUNCTION); }

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

static int32_t print_object(Value value, bool debug) {

  switch (get_type(value)) {
  case OBJ_STRING:
    return ant_string.print(ant_string.from_value(value), debug);
    break;
   case OBJ_FUNCTION:
    return ant_function.print(ant_function.from_value(value), debug);
    break;

  default:
    fprintf(stderr, "Error: Attempted to print object of unkown type.\n");
    return 0;
    break; // Unreachable
  }


}

static void free_object(Object *object) {
  switch (object->type) {
  case OBJ_STRING: {
   ObjectString* string = (ObjectString*)object;
   FREE_ARRAY(char, string->chars, string->length + 1);
   FREE(ObjectString, string);
   break;
  }
   case OBJ_FUNCTION:{
    ObjectFunction* func = (ObjectFunction*)object;
    ant_chunk.free(&func->chunk);
    FREE(ObjectFunction, func);
    break;
  }

   default:
   fprintf(stderr, "Error: Attempted to free object of unkown type: %d\n", object->type);
    break; 
  }
}
