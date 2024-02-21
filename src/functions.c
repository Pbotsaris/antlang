#include "functions.h"
#include <stdio.h>

static ObjectFunction* new_function(void);
static void            print_function(ObjectFunction* function, bool debug);;
static Object*         function_as_object(ObjectFunction* function);
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

static void print_function(ObjectFunction* function, bool _){
   if (function->name == NULL) {
      printf("<script>");
      return;
   }
  printf("<fn %s>", function->name->chars);
}

static ObjectFunction* function_from_value(Value value){
  return (ObjectFunction*)ant_value.as_object(value);
}

static Object* function_as_object(ObjectFunction* function){
  return (Object*)function;
}
