#ifndef ANT_VM_H
#define ANT_VM_H

#include "chunk.h"

typedef enum {
   INTERPRET_OK,
   INTERPRET_COMPILE_ERROR,
   INTERPRET_RUNTIME_ERROR,
}InterpretResult;


typedef struct VM{
   Chunk*      chunk;                    /**< chunk of bytecode instructions */
   uint8_t*    ip;                       /**< instruction pointer */
    // TODO: Dynamic or static stack?
    Value      stack[CONST_STACK_MAX];  /**< The stack used by the interpreter. */
    Value*     stack_top;               /**< The top of the stack. Points to the next available slot */

}VM;

typedef struct VM_API{
   VM*               (*new)(void);
   void              (*free)(VM*);
   InterpretResult   (*interpret)(VM*, Chunk*);
}AntVMAPI;

extern AntVMAPI ant_vm;

#endif //ANT_VM_H
