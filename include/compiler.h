#ifndef ANT_COMPILER_H
#define ANT_COMPILER_H
#include "common.h"
#include "scanner.h"

typedef struct  {
   Scanner *scanner;
}Compiler;

typedef struct AntCompiler{
   Compiler* (*new)(void);
   bool (*compile)(Compiler *compiler, const char* source);
   void (*free)(Compiler* compiler);
}AntCompilerAPI;

extern AntCompilerAPI ant_compiler;
#endif
