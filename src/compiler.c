#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"

#if defined(DEBUG_PRINT_CODE) || defined(DEBUG_TRACE_PARSER)
#include "debug.h"
#endif

#define TRACE_PARSER_ENTER(fmt, ...)
#define TRACE_PARSER_TOKEN(prev, current)
#define TRACE_PARSER_EXIT()                                                    

#ifdef DEBUG_TRACE_PARSER
static int32_t trace_depth = 0;

#undef TRACE_PARSER_ENTER
#undef TRACE_PARSER_EXIT

#define TRACE_PARSER_ENTER(fmt, ...)                                           \
  do {                                                                         \
    ant_debug.trace_parsing(__FUNCTION__, trace_depth++, fmt, ##__VA_ARGS__);  \
  } while (0)

#define TRACE_PARSER_EXIT()                                                    \
  do {                                                                         \
    trace_depth--;                                                             \
  } while (false)

#endif

#if defined(DEBUG_TRACE_PARSER) && defined(DEBUG_TRACE_PARSER_VERBOSE)

#undef TRACE_PARSER_TOKEN

#define TRACE_PARSER_TOKEN(prev, current)                                      \
  do {                                                                         \
    ant_debug.trace_tokens(prev, current, trace_depth);                         \
  } while (0)

#endif



/* Public */
static Compiler *new_compiler(void);
static bool compile(Compiler *compiler, const char *source, Chunk *chunk);
static void free_compiler(Compiler *compiler);

AntCompilerAPI ant_compiler = {
    .new = new_compiler,
    .compile = compile,
    .free = free_compiler,
};

/* Parsing */

static void expression(Compiler *compiler);
static void number(Compiler *compiler);
static void grouping(Compiler *compiler);
static void unary(Compiler *compiler);
static void binary(Compiler *compiler);

/* Parser Rules */

typedef void (*ParserFunc)(Compiler *);

/*  ParseRule: a row in the rules table
 *
 *  ParserRule.prefix:     a function to compile a prefix expression
 *  ParserRule.infix:      a function to compile an infix expression
 *  ParserRule.presedence: the presedence of an infix expression that uses that
 * token as an operator
 *  */

typedef struct {
  ParserFunc prefix;
  ParserFunc infix;
  Presedence presedence;
} ParseRule;

ParseRule rules[] = {
    /* TOKEN TYPE             PREFIX     INFIX   PRESEDENCE */
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    // todo, TOKEN_PLUS as unary?
    [TOKEN_PLUS] = {unary, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LET] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(TokenType type) { return &rules[type]; }

static void parse_pressedence(Compiler *compile, Presedence presedence);

/* Compiling steps */
static void next_token(Compiler *compiler);
static void consume(Compiler *compiler, TokenType type, const char *message);
static void end_of_compilation(Compiler *compiler);

/* Emitting Opcodes and values */
static void emit_return(Compiler *compiler);
static void emit_constant(Compiler *compiler, Value value);

/* Emitting bytecode */
static void emit_byte(Compiler *compiler, uint8_t byte);
static void emit_constant_byte(Compiler *compiler, Value value);
;

/* error handling */
static void error_at(Parser *parser, const char *message);
static void error(Parser *parser, const char *message);
static void error_at_current(Parser *parser, const char *message);

/* utils */
static void reset_parser(Parser *parser);
static const char *precedence_name(Presedence presedence);

/* Compiler API */
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

  compiler->parser->current = (Token){.length = -1, .line = -1, .type = TOKEN_EOF, .start = NULL};
  compiler->parser->prev = compiler->parser->current;

  return compiler;
}

/**/

static bool compile(Compiler *compiler, const char *source, Chunk *chunk) {
  ant_scanner.init(compiler->scanner, source);

  compiler->current_chunk = chunk;
  reset_parser(compiler->parser);

#ifdef DEBUG_TRACE_PARSER
  printf("\n== Parser Trace== \n");
#endif

  next_token(compiler);
  expression(compiler);

  consume(compiler, TOKEN_EOF, "Expect end of expression.");
  end_of_compilation(compiler);

  return !compiler->parser->was_error;
}

/**/

static void free_compiler(Compiler *compiler) {
  if (compiler == NULL)
    return;

  ant_scanner.free(compiler->scanner);
  free(compiler->parser);
  free(compiler);
}

/* Private */

static void next_token(Compiler *compiler) {
  compiler->parser->prev = compiler->parser->current;

  while (true) {
    compiler->parser->current = ant_scanner.scan_token(compiler->scanner);

    if (compiler->parser->current.type != TOKEN_ERROR) {
      break;
    }

    error_at_current(compiler->parser, compiler->parser->current.start);
  }
}

/**/

static void consume(Compiler *compiler, TokenType type, const char *message) {

  if (compiler->parser->current.type == type) {
    next_token(compiler);
    return;
  }

  error_at_current(compiler->parser, message);
}

/**/

static void end_of_compilation(Compiler *compiler) {
  emit_return(compiler);

#ifdef DEBUG_PRINT_CODE
  ant_debug.disassemble_chunk(compiler->current_chunk, "code");
#endif
}

/**/

static void expression(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  /* start with the lowest presedence */
  parse_pressedence(compiler, PREC_ASSIGNMENT);

TRACE_PARSER_EXIT();
}

/**/

static void parse_pressedence(Compiler *compiler, Presedence presedence) {
TRACE_PARSER_ENTER("Compiler *compiler = %p, Presedence presedence = %s",
                     compiler, precedence_name(presedence));

TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  next_token(compiler);

  TokenType prev_type = compiler->parser->prev.type;
  ParserFunc prefix_rule = get_rule(prev_type)->prefix;

  // the first token should always belong to a prefix expression
  if (prefix_rule == NULL) {
    error(compiler->parser, "Expected expression.");
    return;
  }

  prefix_rule(compiler);

  // we only parse infix expressions if the current token has a higher
  // presedence than what was passed to parse_pressedence
  while (presedence <= get_rule(compiler->parser->current.type)->presedence) {
    next_token(compiler);

    TokenType prev_type = compiler->parser->prev.type;
    ParserFunc infix_rule = get_rule(prev_type)->infix;

    infix_rule(compiler);
  }

TRACE_PARSER_EXIT();
}

static void number(Compiler *compiler) {
TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  Value value = (Value)strtod(compiler->parser->prev.start, NULL);
  emit_constant(compiler, value);

TRACE_PARSER_EXIT();
}


/**/

static void grouping(Compiler *compiler) {
TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  expression(compiler);
  consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");

TRACE_PARSER_EXIT();
}

/**/

static void unary(Compiler *compiler) {
TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  TokenType operator_type = compiler->parser->prev.type;

  // expression(compiler);
  parse_pressedence(compiler, PREC_UNARY);

  switch (operator_type) {

  case TOKEN_MINUS:
    emit_byte(compiler, OP_NEGATE);
  case TOKEN_PLUS:
    emit_byte(compiler, OP_POSITIVE);
    break;

  default:
    return; /* unreachable */
  }

TRACE_PARSER_EXIT();
}
/**/

static void binary(Compiler *compiler) {
TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  TokenType operator_type = compiler->parser->prev.type;
  ParseRule *rule = get_rule(operator_type);

  /* call parse presendence with one level higher because binary operators are
   * left associative */
  parse_pressedence(compiler, (Presedence)(rule->presedence + 1));

  switch (operator_type) {
  case TOKEN_PLUS:
    emit_byte(compiler, OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(compiler, OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(compiler, OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(compiler, OP_DIVIDE);
    break;
  default:
    return; /* unreachable */
  }

TRACE_PARSER_EXIT();
}

/**/

static void emit_return(Compiler *compiler) { emit_byte(compiler, OP_RETURN); }

/**/

static void emit_constant(Compiler *compiler, Value value) {
  emit_constant_byte(compiler, value);
}

/**/

static void emit_constant_byte(Compiler *compiler, Value value) {
  int32_t line = compiler->parser->prev.line;
  bool result = ant_chunk.write_constant(compiler->current_chunk, value, line);

  if (!result) {
    error(compiler->parser, "Too many constants in one chunk.");
  }
}

/**/

static void emit_byte(Compiler *compiler, uint8_t byte) {
  int32_t line = compiler->parser->prev.line;
  ant_chunk.write(compiler->current_chunk, byte, line);
}

/**/

static void error(Parser *parser, const char *message) {
  error_at(parser, message);
}

/**/

static void error_at_current(Parser *parser, const char *message) {
  error_at(parser, message);
}

/**/

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

/**/

static void reset_parser(Parser *parser) {
  parser->panic_mode = false;
  parser->was_error = false;
}

/**/

static const char *precedence_name(Presedence precedence) {
  switch (precedence) {
  case PREC_NONE:
    return "PREC_NONE";
  case PREC_ASSIGNMENT:
    return "PREC_ASSIGNMENT";
  case PREC_OR:
    return "PREC_OR";
  case PREC_AND:
    return "PREC_AND";
  case PREC_EQUALITY:
    return "PREC_EQUALITY";
  case PREC_COMPARISON:
    return "PREC_COMPARISON";
  case PREC_TERM:
    return "PREC_TERM";
  case PREC_FACTOR:
    return "PREC_FACTOR";
  case PREC_UNARY:
    return "PREC_UNARY";
  case PREC_CALL:
    return "PREC_CALL";
  case PREC_PRIMARY:
    return "PREC_PRIMARY";
  default:
    return "Unknown Presedence";
  }
}
