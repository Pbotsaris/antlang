#ifndef ANT_OBJECT_H
#define ANT_OBJECT_H

#include "common.h"
#include "value.h"

typedef enum {
  OBJ_STRING = 0,
  OBJ_FUNCTION = 1,
} ObjectType;

struct Object {
  ObjectType type;
  struct Object* next; // for garbage collection
};

typedef struct ObjectAPI {
  ObjectType     (*type)         (Value value);
  bool           (*is_string)    (Value value);
  bool           (*is_function)  (Value value);
  void           (*print)        (Value value, bool debug);
  Object*        (*allocate)     (size_t size, ObjectType object_type);

  void           (*free)         (Object* object);
}ObjectAPI;

extern ObjectAPI ant_object;
#endif // ANT_OBJECT_H
