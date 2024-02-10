#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "strings.h"

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
    ant_debug.trace_tokens(prev, current, trace_depth);                        \
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

static void declaration(Compiler *compiler);
static void variable_declaration(Compiler *compiler);

static void statement(Compiler *compiler);
static void print_statement(Compiler *compiler);
static void expression_statement(Compiler *compiler);

static void expression(Compiler *compiler);
static void number(Compiler *compiler);
static void grouping(Compiler *compiler);
static void unary(Compiler *compiler);
static void binary(Compiler *compiler);
static void literal(Compiler *compiler);
static void string(Compiler *compiler);

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
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_LET] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void parse_pressedence(Compiler *compile, Presedence presedence);

/* variables */
static void define_variable(Compiler *compiler, int32_t index);
static int32_t parse_variable(Compiler *compiler, const char *message);
static int32_t make_identifier_constant(Compiler *compiler, Token *token);

/* Compiling steps */
static void next_token(Compiler *compiler);
static void consume(Compiler *compiler, TokenType type, const char *message);
static void end_of_compilation(Compiler *compiler);
static void synchronize(Compiler *compiler);

/* Emitting Opcodes and values */
static void emit_constant(Compiler *compiler, Value value);
static void emit_global(Compiler *compiler, int32_t index);
static void emit_byte(Compiler *compiler, uint8_t byte);
static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2);

/* error handling */
static void error_at(Parser *parser, const char *message);
static void error(Parser *parser, const char *message);
static void error_at_current(Parser *parser, const char *message);

/* utils */
static void reset_parser(Parser *parser);
static const char *precedence_name(Presedence presedence);
static bool match(Compiler *compiler, TokenType type);
static bool check(Compiler *compiler, TokenType type);
static ParseRule *get_rule(TokenType type);

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

  compiler->parser->current =
      (Token){.length = -1, .line = -1, .type = TOKEN_EOF, .start = NULL};
  compiler->parser->prev = compiler->parser->current;

  return compiler;
}

/**/

static void free_compiler(Compiler *compiler) {
  if (compiler == NULL)
    return;

  ant_scanner.free(compiler->scanner);
  free(compiler->parser);
  free(compiler);
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

  while (!match(compiler, TOKEN_EOF)) {
    declaration(compiler);
  }

  end_of_compilation(compiler);
  return !compiler->parser->was_error;
}

/**/

static void declaration(Compiler *compiler) {
  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  if (match(compiler, TOKEN_LET)) {
    variable_declaration(compiler);

  } else {
    statement(compiler);
  }

  if (parser->panic_mode) {
    synchronize(compiler); // to next statement
  }

  TRACE_PARSER_EXIT();
}

/**/

static void variable_declaration(Compiler *compiler) {
  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  // note that we do not emit a constant instruction here
  // that happens in define_variable below
  uint8_t global_chunk_index =
      parse_variable(compiler, "Expected variable name.");

  if (match(compiler, TOKEN_EQUAL)) {
    expression(compiler);
  } else {
    // implicit initialization to nil
    emit_byte(compiler, OP_NIL);
  }

  consume(compiler, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  // actually emits instruction to define the variable
  define_variable(compiler, global_chunk_index);
  TRACE_PARSER_EXIT();
}

/**/

static void statement(Compiler *compiler) {

  Parser *parser = compiler->parser;
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  if (match(compiler, TOKEN_PRINT)) {
    print_statement(compiler);

  } else {
    expression_statement(compiler);
  }

  TRACE_PARSER_EXIT();
}

/**/

static void print_statement(Compiler *compiler) {

  Parser *parser = compiler->parser;
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  // parse expression first so when print is called there's a value on top of
  // the stack
  expression(compiler);

  // TODO: Maybe remove semicolon from the language
  consume(compiler, TOKEN_SEMICOLON, "Expected ';' after value");
  emit_byte(compiler, OP_PRINT);

  TRACE_PARSER_EXIT();
}

static void expression_statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  expression(compiler);
  consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(compiler, OP_POP);
  TRACE_PARSER_EXIT();
}

static void expression(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser->prev, compiler->parser->current);

  /* start with the lowest presedence */
  parse_pressedence(compiler, PREC_ASSIGNMENT);

  TRACE_PARSER_EXIT();
}

/**/

static void parse_pressedence(Compiler *compiler, Presedence presedence) {

  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p, Presedence presedence = %s",
                     compiler, precedence_name(presedence));

  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  next_token(compiler);

  TokenType prev_type = parser->prev.type;
  ParserFunc prefix_rule = get_rule(prev_type)->prefix;

  // the first token should always belong to a prefix expression
  if (prefix_rule == NULL) {
    error(compiler->parser, "Expected expression.");
    return;
  }

  prefix_rule(compiler);

  // we only parse infix expressions if the current token has a higher
  // presedence than what was passed to parse_pressedence
  while (presedence <= get_rule(parser->current.type)->presedence) {
    next_token(compiler);

    TokenType prev_type = parser->prev.type;
    ParserFunc infix_rule = get_rule(prev_type)->infix;

    infix_rule(compiler);
  }

  TRACE_PARSER_EXIT();
}

static void number(Compiler *compiler) {

  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  double number = strtod(parser->prev.start, NULL);
  emit_constant(compiler, ant_value.from_number(number));

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

  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  TokenType operator_type = parser->prev.type;

  parse_pressedence(compiler, PREC_UNARY);
  switch (operator_type) {

  case TOKEN_MINUS:
    emit_byte(compiler, OP_NEGATE);
  case TOKEN_PLUS:
    emit_byte(compiler, OP_POSITIVE);
  case TOKEN_BANG:
    emit_byte(compiler, OP_NOT);
    break;

  default:
    return; /* unreachable */
  }

  TRACE_PARSER_EXIT();
}
/**/

static void binary(Compiler *compiler) {

  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  TokenType operator_type = parser->prev.type;
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
  case TOKEN_EQUAL_EQUAL:
    emit_byte(compiler, OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emit_byte(compiler, OP_GREATER);
    break;
  case TOKEN_LESS:
    emit_byte(compiler, OP_LESS);
    break;
  case TOKEN_BANG_EQUAL:
    emit_two_bytes(compiler, OP_LESS, OP_NOT);
    break;
  /* because a <= b  is the same as !(a > b) */
  case TOKEN_LESS_EQUAL:
    emit_two_bytes(compiler, OP_GREATER, OP_NOT);
    break;
  /* because a >= b  is the same as !(a < b) */
  case TOKEN_GREATER_EQUAL:
    emit_two_bytes(compiler, OP_LESS, OP_NOT);
    break;
  default:
    return; /* unreachable */
  }

  TRACE_PARSER_EXIT();
}

/* */

static void literal(Compiler *compiler) {
  Parser *parser = compiler->parser;
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  switch (parser->prev.type) {
  case TOKEN_FALSE:
    emit_byte(compiler, OP_FALSE);
    break;
  case TOKEN_NIL:
    emit_byte(compiler, OP_NIL);
    break;
  case TOKEN_TRUE:
    emit_byte(compiler, OP_TRUE);
    break;
  default:
    return; /* unreachable */
  }

  TRACE_PARSER_EXIT();
}

/**/

// TODO STOP ON 19.4

void string(Compiler *compiler) {

  Parser *parser = compiler->parser;

  const char *chars = parser->prev.start + 1; // skip the first quote
  int32_t length = parser->prev.length - 2;   // skip the first and last quote
  ObjectString *string = ant_string.make(chars, length);
  Value value = ant_value.from_object(ant_string.as_object(string));

  emit_constant(compiler, value);
}

/* Variables */

static void define_variable(Compiler *compiler, int32_t index) {
  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  emit_global(compiler, index);

  TRACE_PARSER_EXIT();
}

/**/

static int32_t parse_variable(Compiler *compiler, const char *message) {
  Parser *parser = compiler->parser;

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(parser->prev, parser->current);

  consume(compiler, TOKEN_IDENTIFIER, message);
  int32_t index = make_identifier_constant(compiler, &parser->prev);

  TRACE_PARSER_EXIT();
  return index;
}

/* Note that this function stores a constant but doesn't emit
 * emit a constant instruction. So we are returning the index in the
 * constant array so a later instruction can use it.
 * */

static int32_t make_identifier_constant(Compiler *compiler, Token *token) {
  ObjectString *str    = ant_string.make(token->start, token->length);
  Value name           = ant_value.from_object(ant_string.as_object(str));

  return ant_chunk.add_constant(compiler->current_chunk, name);
}

/* Compilation steps */

static void next_token(Compiler *compiler) {

  Parser *parser = compiler->parser;
  parser->prev = parser->current;

  while (true) {
    parser->current = ant_scanner.scan_token(compiler->scanner);

    if (parser->current.type != TOKEN_ERROR) {
      break;
    }

    error_at_current(parser, parser->current.start);
  }
}

/**/

static void consume(Compiler *compiler, TokenType type, const char *message) {

  Parser *parser = compiler->parser;

  if (parser->current.type == type) {
    next_token(compiler);
    return;
  }

  error_at_current(parser, message);
}

/**/

static void end_of_compilation(Compiler *compiler) {
  emit_byte(compiler, OP_RETURN);

#ifdef DEBUG_PRINT_CODE
  ant_debug.disassemble_chunk(compiler->current_chunk, "code");
#endif
}

/**/

static void synchronize(Compiler *compiler) {
  Parser *parser = compiler->parser;

  parser->panic_mode = false;

  /* syncronize parser to the next statement */
  while (parser->current.type != TOKEN_EOF) {

    if (parser->prev.type == TOKEN_SEMICOLON) {
      return;
    }

    switch (parser->current.type) {
    case TOKEN_CLASS:
    case TOKEN_FN:
    case TOKEN_LET:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    default:;
    }

    next_token(compiler);
  }
}

/**/

static void emit_constant(Compiler *compiler, Value value) {
  int32_t line = compiler->parser->prev.line;
  bool valid = ant_chunk.write_constant(compiler->current_chunk, value, line);

  if (!valid) {
    error(compiler->parser, "Too many constants in one chunk.");
  }
}

/**/

static void emit_global(Compiler *compiler, int32_t index) {
  int32_t line = compiler->parser->prev.line;
  bool valid  = ant_chunk.write_global(compiler->current_chunk, index, line);

  if (!valid) {
    error(compiler->parser, "Too many global variables in one chunk.");
  }
}

/**/

static void emit_byte(Compiler *compiler, uint8_t byte) {
  int32_t line = compiler->parser->prev.line;
  ant_chunk.write(compiler->current_chunk, byte, line);
}

/**/

static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2) {
  int32_t line = compiler->parser->prev.line;
  ant_chunk.write(compiler->current_chunk, byte1, line);
  ant_chunk.write(compiler->current_chunk, byte2, line);
}

/* */

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

static ParseRule *get_rule(TokenType type) { return &rules[type]; }

/* */

static bool match(Compiler *compiler, TokenType type) {
  if (!check(compiler, type))
    return false;

  next_token(compiler);
  return true;
}

static bool check(Compiler *compiler, TokenType type) {
  return compiler->parser->current.type == type;
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
