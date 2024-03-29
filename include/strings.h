#ifndef ANT_STRING_H
#define ANT_STRING_H

#include "value.h"
#include "object.h"
#include "table.h"

/*  NOTE: having the struct Object as the frist member of the struct
 *  allow us to cast a ObjectString pointer to a Object pointer
 *  anf vice versa.
 *  */
struct ObjectString {
  Object  object; /* ATTENTION: this must be the first member of the struct */
  char *  chars;
  int32_t length;
  uint32_t hash; /* cache of hash value */
};

typedef struct {
   ObjectString*  (*new)              (const char *chars, int32_t length);
   void           (*free_table)       (void);
   ObjectString*  (*from_value)       (Value value);
   ObjectString*  (*concat)           (Value a, Value b);
   char*          (*as_cstring)       (ObjectString* string);
   int32_t        (*print)            (ObjectString* string, bool debug);
   Object*        (*as_object)        (ObjectString* string);
}StringAPI;

#define STRING_AS_OBJECT(string) ((Object*)(string))
#define STRING_AS_CSTRING(string) ((string)->chars)
#define STRING_FROM_VALUE(value) ((ObjectString*)VALUE_AS_OBJECT(value))

extern StringAPI ant_string;
extern Table strings;

#endif // ANT_STRING_H
