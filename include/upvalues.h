#ifndef ANT_UPVALUE_H
#define ANT_UPVALUE_H
#include "functions.h"

/* ObjectUpvalues refers the the upvalue used at runtime by the VM. */

typedef struct {
   Object object;
   Value *location; // reference to a variable 
}ObjectUpvalue;

#define UPVALUE_AS_OBJECT(upvalue) ((Object*)(upvalue))
#define UPVALUE_FROM_VALUE(value) ((ObjectUpvalue*)VALUE_AS_OBJECT(value))

typedef struct {
   ObjectUpvalue* (*new_object)(Value *slot);
   ObjectUpvalue* (*capture)(Value *slot);
}UpvalueAPI;

const extern UpvalueAPI ant_upvalues;
#endif // ANT_UPVALUE_H
