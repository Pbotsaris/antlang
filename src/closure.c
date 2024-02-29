#include "closure.h"

static ObjectClosure*  new_closure(ObjectFunction* function);
static ObjectClosure*  closure_from_value(Value value);

const ClosureAPI ant_closure = {
   .new = new_closure,
   .from_value = closure_from_value,
};

static ObjectClosure* new_closure(ObjectFunction* func){

   ObjectClosure *closure = (ObjectClosure*)ant_object.allocate(sizeof(ObjectClosure), OBJ_CLOSURE);
   closure->func          = func;
   size_t size            = sizeof(ObjectUpvalue*) * func->upvalue_count;
   closure->upvalues      = size != 0 ? (ObjectUpvalue**)ant_object.allocate(size, OBJ_UPVALUE) : NULL;
   closure->upvalue_count = func->upvalue_count;

   for (int32_t i = 0; i < func->upvalue_count; i++){
      closure->upvalues[i] = NULL;
   }

  return closure;
}

static ObjectClosure* closure_from_value(Value value){
  return (ObjectClosure*)ant_value.as_object(value);
}
