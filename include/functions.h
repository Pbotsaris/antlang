#ifndef ANT_FUCTIONS_H
#define ANT_FUCTIONS_H

#include "value.h"
#include "chunk.h"
#include "object.h"
#include "strings.h"

struct ObjectFunction {
   Object object; // object header for polymorphism
   int32_t arity;
   Chunk chunk;
   ObjectString *name;
};

 struct ObjectClosure {
   Object object;
   ObjectFunction* func;
};

typedef struct {
   ObjectFunction* (*new)(void);
   ObjectClosure*  (*new_closure)(ObjectFunction* func);
   ObjectFunction* (*from_value)(Value value);
   ObjectClosure*  (*closure_from_value)(Value value);
   Object*         (*as_object)(ObjectFunction* function);
   int32_t         (*print)(ObjectFunction* function);
}ObjectFunctionAPI;

#define FUNCTION_AS_OBJECT(function) ((Object*)(function))
#define FUNCTION_CLOSURE_AS_OBJECT(closure) ((Object*)(closure))
#define FUNCTION_FROM_VALUE(value) ((ObjectFunction*)VALUE_AS_OBJECT(value))
#define FUNCTION_CLOSURE_FROM_VALUE(value) ((ObjectClosure*)VALUE_AS_OBJECT(value))

const extern ObjectFunctionAPI ant_function;

#endif //ANT_FUCTIONS_H
