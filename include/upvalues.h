#ifndef ANT_UPVALUE_H
#define ANT_UPVALUE_H
#include "functions.h"

/* ObjectUpvalues refers the the upvalue used at runtime by the VM. */

typedef struct ObjectUpvalue {
   Object object;
   Value *location; // reference to a variable 
   Value closed; // value of the variable when the upvalue was created
   struct ObjectUpvalue *next; // linked list of open upvalues for closures sharing the same upvalue
   
}ObjectUpvalue;

typedef struct  {
   ObjectUpvalue *head;
}UpvalueList;

#define UPVALUE_AS_OBJECT(upvalue) ((Object*)(upvalue))
#define UPVALUE_FROM_VALUE(value) ((ObjectUpvalue*)VALUE_AS_OBJECT(value))

typedef struct {
   ObjectUpvalue* (*new)(Value *slot);
   ObjectUpvalue* (*capture)(UpvalueList *open_upvalues, Value *stack_slot);
   void           (*close) (UpvalueList *open_upvalues, Value *stack_slot);
}UpvalueAPI;

const extern UpvalueAPI ant_upvalues;
#endif // ANT_UPVALUE_H
