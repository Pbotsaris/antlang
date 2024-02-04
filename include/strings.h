#ifndef ANT_STRING_H
#define ANT_STRING_H

#include "value.h"

typedef struct {
   ObjectString*  (*concat)           (Value a, Value b);
   char*          (*value_as_cstring) (Value value);
   ObjectString*  (*from_value)       (Value value);
   ObjectString*  (*make)             (const char *chars, int32_t length);
   void           (*print)            (Value value);
   Object*        (*as_object)        (ObjectString* string);
   bool            (*equals)          (Value a, Value b);
}StringAPI;

extern StringAPI ant_string;

#endif // ANT_STRING_H
