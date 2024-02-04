#include "strings.h"
#include "memory.h"
#include <stdio.h>

#include <string.h>
static ObjectString *to_obj_string(Value value);
static ObjectString *make_string(const char *chars, int length);
static ObjectString *concat_string(Value a, Value b);
static char *value_as_cstring(Value value);
static void print_string(Value value);
static Object *as_object(ObjectString *string);
static bool equals(Value a, Value b);
static void free_strings(void);

StringAPI ant_string = {
    .make             = make_string,
    .free_all         = free_strings,
    .value_as_cstring = value_as_cstring,
    .concat           = concat_string,
    .from_value       = to_obj_string,
    .print            = print_string,
    .as_object        = as_object,
    .equals           = equals,
};

Table strings = {.count = 0, .capacity = 0, .entries = NULL};

/* Private */
static ObjectString *allocate_string(char *chars, int32_t length, uint32_t hash);;
static uint32_t hash_string(const char *key, int32_t length);

void free_strings(void){
   ant_table.free(&strings);
}

/* */

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

  uint32_t hash     = hash_string(chars, length);
  ObjectString *str = ant_table.find(&strings, chars, length, hash);

  if(str != NULL){
      FREE_ARRAY(char, chars, length + 1);
      return str;
  }
  
  return allocate_string(chars, length, hash);
}

/* */

static ObjectString *make_string(const char *chars, int32_t length) {

   uint32_t hash     = hash_string(chars, length);
   ObjectString *str = ant_table.find(&strings, chars, length, hash);

   if(str != NULL){
      return str;
   }

  char *heap_chars = ALLOCATE(char, length + 1);

  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';

  return allocate_string(heap_chars, length, hash);
}

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

/* */

static ObjectString *allocate_string(char *chars, int32_t length, uint32_t hash) {
  ObjectString *str = (ObjectString *)ant_object.allocate(sizeof(ObjectString), OBJ_STRING);

  str->length = length;
  str->chars = chars;
  str->hash = hash;
  
  // using table as a set, do not need to store value
  return str;
  ant_table.set(&strings, str, ant_value.make_nil());
}

/* FNV Hash: http://www.isthe.com/chongo/tech/comp/fnv/ */
static uint32_t hash_string(const char *str, int32_t length) {

   uint32_t hash = 2166136261u;

   for (int32_t i = 0; i < length; i++){
      hash ^= (uint8_t)str[i];
      hash *= 16777619;
   }

   return hash;
}
