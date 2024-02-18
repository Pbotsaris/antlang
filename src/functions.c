#include "functions.h"
#include <stdio.h>

static ObjectFunction* new_function(void);
static void            print_function(ObjectFunction* function, bool debug);;

const ObjectFunctionAPI ant_function = {
   .new = new_function,
   .print = print_function,

};

static ObjectFunction *new_function(void){
  ObjectFunction* func = (ObjectFunction*)ant_object.allocate(sizeof(ObjectFunction), OBJ_FUNCTION);
  func->arity = 0;
  func->name = NULL;
  ant_chunk.init(&func->chunk);
  return func;
}

static void print_function(ObjectFunction* function, bool _){
  printf("<fn %s>", function->name->chars);
}
