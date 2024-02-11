#ifndef ANT_COMPILER_H
#define ANT_COMPILER_H
#include "chunk.h"
#include "common.h"
#include "scanner.h"
#include "table.h"
#include "var_mapping.h"
#include "parser.h"

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

typedef struct {
  Scanner *scanner;
  Parser parser;
  Chunk *current_chunk;
  VarMapping globals;
} Compiler;

typedef struct AntCompiler {
  Compiler *(*new)(void);
  bool (*compile)(Compiler *compiler, const char *source, Chunk *chunk);
  void (*free)(Compiler *compiler);
} AntCompilerAPI;

const extern AntCompilerAPI ant_compiler;
#endif
