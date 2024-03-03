#include "upvalues.h"
#include <stdio.h>

static void init_upvalues(Upvalues *upvalues, ObjectFunction *func);
static ObjectUpvalue* new_upvalue_runtime_object(Value *slot);
static ObjectUpvalue *capture_upvalue(Value *slot);
static int32_t add_upvalue(Upvalues *upvalues, uint8_t, bool is_local);

const UpvalueAPI ant_upvalues = {
   .init = init_upvalues,
   .new_object = new_upvalue_runtime_object,
   .capture = capture_upvalue,
   .add = add_upvalue,
};

static void init_upvalues(Upvalues *upvalues, ObjectFunction *func){
   upvalues->func = func;
}

static ObjectUpvalue* new_upvalue_runtime_object(Value *slot){
   ObjectUpvalue *upvalue = (ObjectUpvalue*)ant_object.allocate(sizeof(ObjectUpvalue), OBJ_UPVALUE);
   upvalue->location = slot;
   return upvalue;
}

static ObjectUpvalue *capture_upvalue(Value *slot){
   ObjectUpvalue *upvalue = new_upvalue_runtime_object(slot);
   return upvalue;
}

static int32_t add_upvalue(Upvalues *upvalues, uint8_t index, bool is_local){

   if(upvalues->func == NULL){
      return UPVALUE_NOT_INITIALIZED;
   }

   if(upvalues->func->upvalue_count == OPTION_UPVALUE_MAX){
      return UPVALUE_REACHED_MAX;
   }

   int32_t up_count = upvalues->func->upvalue_count;

   /* does the upvalue already exist? */
   for(int32_t i = 0; i < up_count; i++){
      Upvalue *upvalue = &upvalues->values[i];
      if(upvalue->index == index && upvalue->is_local == is_local){
         return i;
      }
   }

   upvalues->values[up_count].is_local = is_local;
   upvalues->values[up_count].index = index; // closing over the local variable index
   upvalues->func->upvalue_count++;

   return up_count;
}
