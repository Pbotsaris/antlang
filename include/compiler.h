#ifndef ANT_COMPILER_H
#define ANT_COMPILER_H
#include "chunk.h"
#include "common.h"
#include "scanner.h"

typedef struct {
   Token current;
   Token prev;
   bool was_error;
   bool panic_mode;
}Parser;


typedef struct  {
   Scanner *scanner;
   Parser *parser;
   Chunk *current_chunk;
}Compiler;

typedef struct AntCompiler{
   Compiler* (*new)(void);
   bool (*compile)(Compiler *compiler, const char* source, Chunk *chunk);
   void (*free)(Compiler* compiler);
}AntCompilerAPI;

extern AntCompilerAPI ant_compiler;
#endif
