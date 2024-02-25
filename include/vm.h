#ifndef ANT_VM_H
#define ANT_VM_H

#include "compiler.h"
#include "config.h"
#include "stack.h"

typedef enum {
   INTERPRET_OK,
   INTERPRET_COMPILE_ERROR,
   INTERPRET_RUNTIME_ERROR,
}InterpretResult;


typedef struct {
   ObjectFunction* func;  /* function being called */
   Value*          slots; /* first slot of the call frame */
   uint8_t*        ip;    /* return address */
}CallFrame;

typedef struct VM{
   Stack       stack;
   Compiler    compiler;            
   ValueArray  globals;
   CallFrame   frames[OPTION_FRAMES_MAX];
   int32_t     frame_count;
}VM;

typedef struct VM_API{
   VM*               (*new)(void);
   void              (*free)(VM*);
   void              (*repl)(VM*);
   InterpretResult   (*interpret)(VM*, const char*);
}AntVMAPI;

extern AntVMAPI ant_vm;

#endif //ANT_VM_H
