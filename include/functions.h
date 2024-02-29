#ifndef ANT_FUCTIONS_H
#define ANT_FUCTIONS_H

#include "value.h"
#include "chunk.h"
#include "object.h"
#include "strings.h"

struct ObjectFunction {
   Object object; // object header for polymorphism
   int32_t arity;
   int32_t upvalue_count;
   Chunk chunk;
   ObjectString *name;
};

typedef struct {
   ObjectFunction* (*new)(void);
   ObjectFunction* (*from_value)(Value value);
   Object*         (*as_object)(ObjectFunction* function);
   int32_t         (*print)(ObjectFunction* function);
}ObjectFunctionAPI;

#define FUNCTION_AS_OBJECT(function) ((Object*)(function))
#define FUNCTION_FROM_VALUE(value) ((ObjectFunction*)VALUE_AS_OBJECT(value))

const extern ObjectFunctionAPI ant_function;

#endif //ANT_FUCTIONS_H
