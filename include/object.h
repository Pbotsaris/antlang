#ifndef ANT_OBJECT_H
#define ANT_OBJECT_H

#include "common.h"
#include "value.h"

typedef enum {
  OBJ_STRING = 0,
  OBJ_FUNCTION = 1,
  OBJ_CLOSURE = 2,
  OBJ_NATIVE = 3,
} ObjectType;

struct Object {
  ObjectType type;
  struct Object* next; // for garbage collection
};

typedef struct ObjectAPI {
  ObjectType     (*type)         (Value value);
  bool           (*is_string)    (Value value);
  bool           (*is_function)  (Value value);
  bool           (*is_closure)   (Value value);
  bool           (*is_native)    (Value value);
  int32_t        (*print)        (Value value, bool debug);
  Object*        (*allocate)     (size_t size, ObjectType object_type);
  void           (*free)         (Object* object);
}ObjectAPI;

#define OBJECT_TYPE(value)          (VALUE_AS_OBJECT((value))->type)
#define OBJECT_IS_TYPE(value, type) (VALUE_IS_OBJECT((value)) && OBJECT_TYPE((value)) == (type))
#define OBJECT_IS_STRING(value)     (OBJECT_IS_TYPE((value), OBJ_STRING))
#define OBJECT_IS_FUNCTION(value)   (OBJECT_IS_TYPE((value), OBJ_FUNCTION))
#define OBJECT_IS_CLOSURE(value)    (OBJECT_IS_TYPE((value), OBJ_CLOSURE))
#define OBJECT_IS_NATIVE(value)     (OBJECT_IS_TYPE((value), OBJ_NATIVE))

extern ObjectAPI ant_object;
#endif // ANT_OBJECT_H
