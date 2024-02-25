#include "functions.h"
#include <stdio.h>

static ObjectFunction* new_function(void);
static int32_t         print_function(ObjectFunction* func);
static Object*         function_as_object(ObjectFunction* func);
static ObjectFunction* function_from_value(Value value);

const ObjectFunctionAPI ant_function = {
   .new = new_function,
   .print = print_function,
   .as_object = function_as_object,
   .from_value = function_from_value,
};

static ObjectFunction *new_function(void){
  ObjectFunction* func = (ObjectFunction*)ant_object.allocate(sizeof(ObjectFunction), OBJ_FUNCTION);
  func->arity = 0;
  func->name = NULL;
  ant_chunk.init(&func->chunk);
  return func;
}

static int32_t print_function(ObjectFunction* func){

   if (func->name == NULL) {
      return printf("<script>");
   }
  return printf("<fn %s>", func->name->chars);
}

static ObjectFunction* function_from_value(Value value){
  return (ObjectFunction*)ant_value.as_object(value);
}

static Object* function_as_object(ObjectFunction* function){
  return (Object*)function;
}
