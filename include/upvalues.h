#ifndef ANT_UPVALUE_H
#define ANT_UPVALUE_H
#include "common.h"
#include "functions.h"
#include "config.h"

/* Runtime object */

typedef struct {
   Object object;
   Value *location; // reference to a variable 
}ObjectUpvalue;

#define UPVALUE_AS_OBJECT(upvalue) ((Object*)(upvalue))
#define UPVALUE_FROM_VALUE(value) ((ObjectUpvalue*)VALUE_AS_OBJECT(value))

/* Compile time */

#define UPVALUE_REACHED_MAX -3
#define UPVALUE_NOT_INITIALIZED -4

typedef struct {
   uint8_t index;
   bool is_local;
}Upvalue;

typedef struct {
  Upvalue values[OPTION_UPVALUE_MAX];

  /* we need the function in this structure because upvalue_count is stored
   * within the function as we need to access it during runtime.
   * */

  ObjectFunction *func;
}Upvalues;


typedef struct {
   ObjectUpvalue* (*new_object)(Value *slot);
   ObjectUpvalue* (*capture)(Value *slot);
   void    (*init)(Upvalues *upvalues, ObjectFunction *function);
   int32_t (*add)(Upvalues *upvalues, uint8_t index, bool is_local);
}UpvalueAPI;

const extern UpvalueAPI ant_upvalues;
#endif // ANT_UPVALUE_H
