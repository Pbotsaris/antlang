#ifndef ANT_COMPILATION_UPVALUES_H
#define ANT_COMPILATION_UPVALUES_H

#include "common.h"
#include "functions.h"
#include "config.h"

/* This file references the upvalues data structure used by the compiler to keep
 * track of the upvalues in index positions in the VM stack. when the compiler finds
 * a variable being referenced in a function, it will look for the variable in the
 * enclosing functions and if it finds it, it will add it to the upvalues list of the
 * function being compiled. 
 *
 * When the compilation is done, the enclosing compiler will emit the upvalues via OP_CLOSURE
 * instruction. 
 *
 * Do not confuse this with the upvalue struct used by VM.
 * */

#define UPVALUE_REACHED_MAX -3
#define UPVALUE_NOT_INITIALIZED -4

typedef struct {
   uint8_t index;
   bool is_local;
}CompilerUpvalue;

typedef struct {
  CompilerUpvalue values[OPTION_UPVALUE_MAX];

  /* we need the function in this structure because upvalue_count is stored
   * within the function as we need to access it during runtime.
   * */

  ObjectFunction *func;
}CompilerUpvalues;


typedef struct {
   void    (*init)(CompilerUpvalues *upvalues, ObjectFunction *function);
   int32_t (*add)(CompilerUpvalues *upvalues, uint8_t index, bool is_local);
}CompilerUpvalueAPI;

const extern CompilerUpvalueAPI ant_compiler_upvalues;
#endif // ANT_UPVALUE_H



