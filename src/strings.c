#include "strings.h"
#include "memory.h"
#include "object.h"
#include <stdio.h>
#include <string.h>

static ObjectString *to_obj_string(Value value);
static ObjectString *make_string(const char *chars, int length);
static ObjectString *concat_string(Value a, Value b);
static char *value_as_cstring(Value value);
static void print_string(Value value);
static Object *as_object(ObjectString *string);
static bool equals(Value a, Value b);

/* Private */
static ObjectString *allocate_string(char *chars, int32_t length);

StringAPI ant_string = {
    .make = make_string,
    .value_as_cstring = value_as_cstring,
    .concat = concat_string,
    .from_value = to_obj_string,
    .print = print_string,
    .as_object = as_object,
    .equals = equals,
};

static ObjectString *to_obj_string(Value value) {
  return (ObjectString *)ant_value.as_object(value);
}

/* */

static char *value_as_cstring(Value value) {
  return to_obj_string(value)->chars;
}

/* */

static ObjectString *concat_string(Value a, Value b) {
  ObjectString *sa = to_obj_string(a);
  ObjectString *sb = to_obj_string(b);

  int32_t length = sa->length + sb->length;
  char *chars    = ALLOCATE(char, length + 1);

  memcpy(chars, sa->chars, sa->length);
  memcpy(chars + sa->length, sb->chars, sb->length);

  chars[length] = '\0';

  return allocate_string(chars, length);
}

/* */

static ObjectString *make_string(const char *chars, int length) {
  char *heap_char = ALLOCATE(char, length + 1);

  memcpy(heap_char, chars, length);
  heap_char[length] = '\0';

  return allocate_string(heap_char, length);
}

/* */

static ObjectString *allocate_string(char *chars, int32_t length) {
  ObjectString *str = (ObjectString *)ant_object.allocate(sizeof(ObjectString), OBJ_STRING);

  str->length = length;
  str->chars = chars;
  return str;
}

/* */

void print_string(Value value) {
  printf("{ '%s' }", ant_string.value_as_cstring(value));
}

/* */

Object *as_object(ObjectString *string) { return (Object *)string; }

/* */

static bool equals(Value a, Value b) {
  ObjectString *sa = to_obj_string(a);
  ObjectString *sb = to_obj_string(b);
  return sa->length == sb->length && memcmp(sa->chars, sb->chars, sa->length) == 0;
}
