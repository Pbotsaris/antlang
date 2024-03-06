#include "compilation_upvalues.h"
#include <stdio.h>
#include <stddef.h> 

static void init_upvalues(CompilerUpvalues *upvalues, ObjectFunction *func);
static int32_t add_upvalue(CompilerUpvalues *upvalues, uint8_t, bool is_local);

const CompilerUpvalueAPI ant_compiler_upvalues = {
   .init = init_upvalues,
   .add = add_upvalue,
};

static void init_upvalues(CompilerUpvalues *upvalues, ObjectFunction *func){
   upvalues->func = func;
}

static int32_t add_upvalue(CompilerUpvalues *upvalues, uint8_t index, bool is_local){

   if(upvalues->func == NULL){
      return UPVALUE_NOT_INITIALIZED;
   }

   if(upvalues->func->upvalue_count == OPTION_UPVALUE_MAX){
      return UPVALUE_REACHED_MAX;
   }

   int32_t up_count = upvalues->func->upvalue_count;

   /* does the upvalue already exist? */
   for(int32_t i = 0; i < up_count; i++){
      CompilerUpvalue *upvalue = &upvalues->values[i];
      if(upvalue->index == index && upvalue->is_local == is_local){
         return i;
      }
   }

   upvalues->values[up_count].is_local = is_local;
   upvalues->values[up_count].index = index; // closing over the local variable index
   upvalues->func->upvalue_count++;

   return up_count;
}
