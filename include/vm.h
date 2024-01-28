#ifndef ANT_VM_H
#define ANT_VM_H

#include "chunk.h"

typedef enum {
   INTERPRET_OK,
   INTERPRET_COMPILE_ERROR,
   INTERPRET_RUNTIME_ERROR,
}InterpretResult;


typedef struct VM{
   Chunk*            chunk;
   uint8_t*          ip; /* instruction pointer */
}VM;

typedef struct VM_API{
   VM*               (*new)(void);
   void              (*free)(VM*);
   InterpretResult   (*interpret)(VM*, Chunk*);
}AntVMAPI;

extern AntVMAPI ant_vm;

#endif //ANT_VM_H
