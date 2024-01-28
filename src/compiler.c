#include <stdlib.h>
#include <stdio.h>

#include "compiler.h"

static Compiler* new_compiler(void);
static void compile(Compiler *compiler, const char* source);
static void free_compiler(Compiler* compiler);

AntCompilerAPI ant_compiler = {
    .new = new_compiler,
    .compile = compile,
    .free = free_compiler,
};

static Compiler* new_compiler(void) {
   Compiler* compiler = (Compiler*)malloc(sizeof(Compiler));

   if(compiler == NULL){
      fprintf(stderr, "Could not allocate memory for compiler\n");
      exit(1);
   }

   return compiler;
}

static void compile(Compiler *compiler, const char* source) {

   ant_scanner.init(compiler->scanner, source);
   int32_t line = -1;

   while(true){
      Token token = ant_scanner.scan_token(compiler->scanner);

      if(token.line != line){
         printf("%4d ", token.line);
         line = token.line;
      } else{
         printf("   | ");
      }
      printf("%2d '%.*s'\n", token.type, token.length, token.start);

      if(token.type == TOKEN_EOF) return;
   }
}

static void free_compiler(Compiler* compiler) {
   if(compiler == NULL)
      return;

   ant_scanner.free(compiler->scanner);
   free(compiler);
}

