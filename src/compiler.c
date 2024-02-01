#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"

static Compiler *new_compiler(void);
static bool compile(Compiler *compiler, const char *source, Chunk *chunk);
static void free_compiler(Compiler *compiler);

AntCompilerAPI ant_compiler = {
    .new = new_compiler,
    .compile = compile,
    .free = free_compiler,
};

/* Compiling steps */
static void next(Compiler *compiler);
static void consume(Compiler *compiler, TokenType type, const char *message);
static void end_of_compilation(Compiler *compiler);

static void number(Compiler *compiler);

static void emit_return(Compiler *compiler);
static void emit_constant(Compiler *compiler, Value value);
/* Emitting bytecode */
static void emit_byte(Compiler *compiler, uint8_t byte);
static void emit_constant_byte(Compiler *compiler, Value value);;

/* error handling */
static void error_at(Parser *parser, const char *message);
static void error(Parser *parser, const char *message);
static void error_at_current(Parser *parser, const char *message);

/* utils */
static void reset_parser(Parser *parser);

static Compiler *new_compiler(void) {
  Compiler *compiler = (Compiler *)malloc(sizeof(Compiler));

  if (compiler == NULL) {
    fprintf(stderr, "Could not allocate memory for compiler\n");
    exit(1);
  }

  Parser *parser = (Parser *)malloc(sizeof(Parser));

  if (parser == NULL) {
    fprintf(stderr, "Could not allocate memory for parser\n");
    exit(1);
  }

  compiler->scanner = ant_scanner.new();
  compiler->parser = parser;
  return compiler;
}

static bool compile(Compiler *compiler, const char *source, Chunk *chunk) {
  ant_scanner.init(compiler->scanner, source);

  compiler->current_chunk = chunk;
  reset_parser(compiler->parser);
  next(compiler);
  // expression();
  
  consume(compiler, TOKEN_EOF, "Expect end of expression.");
  end_of_compilation(compiler);
  return !compiler->parser->was_error;
}

static void free_compiler(Compiler *compiler) {
  if (compiler == NULL)
    return;

  ant_scanner.free(compiler->scanner);
  free(compiler->parser);
  free(compiler);
}

static void next(Compiler *compiler) {
  compiler->parser->prev = compiler->parser->current;

  while (true) {
    compiler->parser->current = ant_scanner.scan_token(compiler->scanner);

    if (compiler->parser->current.type != TOKEN_ERROR) {
      break;
    }

    error_at_current(compiler->parser, compiler->parser->current.start);
  }
}

static void consume(Compiler *compiler, TokenType type, const char *message) {

  if (compiler->parser->current.type == type) {
    next(compiler);
    return;
  }

  error_at_current(compiler->parser, message);
}

static void end_of_compilation(Compiler *compiler) { emit_return(compiler); }

static void number(Compiler *compiler){
   Value value = (Value)strtod(compiler->parser->prev.start, NULL);
}

static void emit_return(Compiler *compiler) { emit_byte(compiler, OP_RETURN); }

static void emit_constant(Compiler *compiler, Value value) {
  emit_constant_byte(compiler, value);
}

static void emit_constant_byte(Compiler *compiler, Value value) {
  int32_t line = compiler->parser->prev.line;
  bool result = ant_chunk.write_constant(compiler->current_chunk, value, line);

  if(!result){
    error(compiler->parser, "Too many constants in one chunk.");
  }
}

static void emit_byte(Compiler *compiler, uint8_t byte) {
  int32_t line = compiler->parser->prev.line;
  ant_chunk.write(compiler->current_chunk, byte, line);
}

static void error(Parser *parser, const char *message) {
  error_at(parser, message);
}

static void error_at_current(Parser *parser, const char *message) {
  error_at(parser, message);
}

static void error_at(Parser *parser, const char *message) {

  Token token = parser->current;

  if (parser->panic_mode)
    return; // suppress errors after the first one
  parser->panic_mode = true;

  fprintf(stderr, "[line %d] Error", token.line);

  switch (token.type) {
  case TOKEN_ERROR:
    // nothing for now
    break;
  case TOKEN_EOF:
    fprintf(stderr, " at end\n");
    break;
  default:
    fprintf(stderr, " at '%.*s'\n", token.length, token.start);
    break;
  }

  fprintf(stderr, ": %s\n", message);
  parser->was_error = true;
}

static void reset_parser(Parser *parser) {
  parser->panic_mode = false;
  parser->was_error = false;
}
