#ifndef ANT_COMPILATION_UPVALUES_H
#define ANT_COMPILATION_UPVALUES_H

#include "common.h"
#include "functions.h"
#include "config.h"

/* 
 * This file defines the data structures and APIs for managing upvalues during
 * compilation in the AntLang interpreter. Upvalues are variables from enclosing (outer)
 * function scopes that are referenced within a nested (inner) function. This mechanism
 * allows nested functions to access and modify variables from their outer scope, enabling
 * the implementation of closures.
 *
 * Overview:
 * - The `CompilerUpvalue` structure represents a single upvalue with its index in the VM stack
 *   and a flag indicating if it is a local variable of the enclosing function.
 * - The `CompilerUpvalues` structure maintains a list of such upvalues, alongside a reference
 *   to the function being compiled. This is crucial as the `upvalue_count` is stored within
 *   the function object and must be accessible at runtime for correct closure behavior.
 *
 * The process:
 * During compilation, when a variable referenced in a function is identified as belonging to an
 * enclosing scope, it is added to the current function's upvalue list. This list is then utilized
 * when emitting the `OP_CLOSURE` instruction, which finalizes the function object with correct
 * access to its upvalues.
 *
 * Note:
 * This upvalue management for the compiler should not be confused with the runtime upvalue
 * structures used by the VM. Although related in concept, they serve different purposes within
 * the interpreter's architecture.
 *
 */

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



