#ifndef ANT_VM_H
#define ANT_VM_H

#include "chunk.h"
#include "compiler.h"
#include "table.h"

typedef enum {
   INTERPRET_OK,
   INTERPRET_COMPILE_ERROR,
   INTERPRET_RUNTIME_ERROR,
}InterpretResult;


typedef struct VM{
   Chunk*      chunk;               
   uint8_t*    ip;                   
   Value       stack[OPTION_STACK_MAX]; //TODO: make this dynamic
   Value*      stack_top;            
   Compiler*   compiler;            
   Table       globals;
}VM;

typedef struct VM_API{
   VM*               (*new)(void);
   void              (*free)(VM*);
   void              (*repl)(VM*);
   InterpretResult   (*interpret)(VM*, const char*);
}AntVMAPI;

extern AntVMAPI ant_vm;

#endif //ANT_VM_H
