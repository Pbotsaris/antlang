#ifndef ANT_COMPILER_H
#define ANT_COMPILER_H
#include "scanner.h"
#include "parser.h"
#include "locals.h"
#include "upvalues.h"
#include "common.h"

typedef enum {
  PREC_NONE       = 0,  /* Lowest precedence */
  PREC_ASSIGNMENT = 1, /*       =           */
  PREC_OR         = 2, /*       or          */
  PREC_AND        = 3, /*       and         */
  PREC_EQUALITY   = 4, /*       == !=       */
  PREC_COMPARISON = 5, /*       < > <= >=   */
  PREC_TERM       = 6, /*       + -         */
  PREC_FACTOR     = 7, /*       * /         */
  PREC_UNARY      = 8, /*       ! -         */
  PREC_CALL       = 9, /*       . ()        */
  PREC_PRIMARY   = 10, /*       Highest     */
} Presedence;

typedef enum {
   COMPILATION_TYPE_FUNC,
   COMPILATION_TYPE_SCRIPT,
}CompilationType;;



typedef struct Compiler {
  Scanner scanner;
  Parser parser;
  LocalStack locals;
  Upvalues upvalues;
  ObjectFunction *func;
  CompilationType type;
  struct Compiler *enclosing;
} Compiler;

typedef struct AntCompiler {
  void            (*init)(Compiler *compiler, CompilationType type);
  ObjectFunction* (*compile)(Compiler *compiler, const char *source);
} AntCompilerAPI;

const extern AntCompilerAPI ant_compiler;
#endif
