#ifndef ANT_VM_H
#define ANT_VM_H

#include "chunk.h"
#include "compiler.h"

typedef enum {
   INTERPRET_OK,
   INTERPRET_COMPILE_ERROR,
   INTERPRET_RUNTIME_ERROR,
}InterpretResult;


typedef struct VM{
   Chunk*      chunk;                    /**< chunk of bytecode instructions */
   uint8_t*    ip;                       /**< instruction pointer */
    // TODO: Dynamic or static stack?
   Value      stack[OPTION_STACK_MAX];  /**< The stack used by the interpreter. */
   Value*     stack_top;                /**< The top of the stack. Points to the next available slot */
   Compiler*  compiler;                 /**< The compiler used by the interpreter. */
}VM;

typedef struct VM_API{
   VM*               (*new)(void);
   void              (*free)(VM*);
   void              (*repl)(VM*);
   InterpretResult   (*interpret)(VM*, const char*);
}AntVMAPI;

extern AntVMAPI ant_vm;

#endif //ANT_VM_H
