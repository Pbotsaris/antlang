#include "upvalues.h"

static ObjectUpvalue* new_upvalue_runtime_object(Value *slot);
static ObjectUpvalue *capture_upvalue(Value *slot);

const UpvalueAPI ant_upvalues = {
   .new_object = new_upvalue_runtime_object,
   .capture = capture_upvalue,
};

static ObjectUpvalue* new_upvalue_runtime_object(Value *slot){
   ObjectUpvalue *upvalue = (ObjectUpvalue*)ant_object.allocate(sizeof(ObjectUpvalue), OBJ_UPVALUE);
   upvalue->location = slot;
   return upvalue;
}

static ObjectUpvalue *capture_upvalue(Value *slot){
   ObjectUpvalue *upvalue = new_upvalue_runtime_object(slot);
   return upvalue;
}
