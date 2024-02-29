#ifndef ANT_CLOSURE_H
#define ANT_CLOSURE_H

#include "object.h"
#include "upvalues.h"

 struct ObjectClosure {
   Object object;
   ObjectFunction* func;
   ObjectUpvalue** upvalues;

   /* count is duplicated from ObjectFunction because we need the count in the GC, 
    * and ObjectFunction could be already freed then */
   int32_t upvalue_count; 
};

typedef struct {
   ObjectClosure*  (*new)(ObjectFunction* func);
   ObjectClosure*  (*from_value)(Value value);
}ClosureAPI;

const extern ClosureAPI ant_closure;

#define CLOSURE_AS_OBJECT(closure) ((Object*)(closure))
#define CLOSURE_FROM_VALUE(value) ((ObjectClosure*)VALUE_AS_OBJECT(value))
#endif
