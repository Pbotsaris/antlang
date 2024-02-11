#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "config.h"
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

// write global variable callback
typedef bool (*Callback)(Chunk *chunk, int32_t const_index, int32_t line);

/* Public */
static void init_compiler(Compiler *compiler);
static bool compile(Compiler *compiler, const char *source, Chunk *chunk);
static void free_compiler(Compiler *compiler);

const AntCompilerAPI ant_compiler = {
    .init = init_compiler,
    .compile = compile,
    .free = free_compiler,
};

/* Parsing */
static void declaration(Compiler *compiler);
static void variable_declaration(Compiler *compiler);
static void statement(Compiler *compiler);
static void print_statement(Compiler *compiler);
static void block(Compiler *compiler);
static void expression_statement(Compiler *compiler);
static void expression(Compiler *compiler);

static void number(Compiler *compiler, bool can_assign);
static void grouping(Compiler *compiler, bool can_assign);
;
static void unary(Compiler *compiler, bool can_assign);
static void binary(Compiler *compiler, bool can_assign);
static void variable(Compiler *compiler, bool can_assign);
static void literal(Compiler *compiler, bool can_assign);
static void string(Compiler *compiler, bool can_assign);

/* Parser Rules */

typedef void (*ParserFunc)(Compiler *, bool can_assign);
;

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
    /* TOKEN TYPE          PREFIX     INFIX   PRESEDENCE */
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
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
static void define_global_variable(Compiler *compiler, int32_t index);
static void declare_local_variable(Compiler *compiler);
static void named_variable(Compiler *compiler, Token name, bool can_assign);
static int32_t parse_variable(Compiler *compiler, const char *message);
static int32_t make_global_identifier(Compiler *compiler, Token *token);

/* Block */
static void begin_scope(Compiler *compiler);
static void end_scope(Compiler *compiler);

/* Compiling steps */
static void next_token(Compiler *compiler);
static void consume(Compiler *compiler, TokenType type, const char *message);
static void end_of_compilation(Compiler *compiler);
static void synchronize(Compiler *compiler);

/* Emitting Opcodes and values */
static void emit_constant(Compiler *compiler, Value value);
static void emit_variable(Compiler *compiler, int32_t index, Callback callback);
static void emit_byte(Compiler *compiler, uint8_t byte);
static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2);

/* error handling */
static void error_at(Parser *parser, const char *message);
static void error(Parser *parser, const char *message);
static void error_at_current(Parser *parser, const char *message);

/* utils */
static const char *precedence_name(Presedence presedence);
static bool match(Compiler *compiler, TokenType type);
static bool check(Compiler *compiler, TokenType type);
static ParseRule *get_rule(TokenType type);
static bool is_global(int32_t local_index);

/* Compiler API */
static void init_compiler(Compiler *compiler) {

  /* scanner gets initialize on compile method */
  ant_parser.init(&compiler->parser);
  ant_mapping.init(&compiler->globals);
  ant_locals.init(&compiler->locals);

  /* current_chunk gets populate in compile method */
  compiler->current_chunk = NULL;
}

/**/

static void free_compiler(Compiler *compiler) {
  /* current_chunk gets freed in vm */
  ant_mapping.free(&compiler->globals);
}

/**/

static bool compile(Compiler *compiler, const char *source, Chunk *chunk) {
  ant_scanner.init(&compiler->scanner, source);

  compiler->current_chunk = chunk;
  ant_parser.reset(&compiler->parser);

#ifdef DEBUG_TRACE_PARSER
  printf("\n== Parser Trace== \n");
#endif

  next_token(compiler);

  while (!match(compiler, TOKEN_EOF)) {
    declaration(compiler);
  }

  end_of_compilation(compiler);
  return !compiler->parser.was_error;
}

/**/

static void declaration(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  if (match(compiler, TOKEN_LET)) {
    variable_declaration(compiler);

  } else {
    statement(compiler);
  }

  if (compiler->parser.panic_mode) {
    synchronize(compiler); // to next statement
  }

  TRACE_PARSER_EXIT();
}

/**/

static void variable_declaration(Compiler *compiler) {

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  // note that we do not emit a constant instruction here
  // that happens in define_variable below
  uint8_t globals_index = parse_variable(compiler, "Expected variable name.");

  if (match(compiler, TOKEN_EQUAL)) {
    expression(compiler);
  } else {
    // implicit initialization to nil
    emit_byte(compiler, OP_NIL);
  }

  consume(compiler, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  if(compiler->locals.depth > 0){
   // we have a local variable
    TRACE_PARSER_EXIT();
    return;
  }

  // actually emits instruction to define a global variable
  define_global_variable(compiler, globals_index);
  TRACE_PARSER_EXIT();
}

/**/

static void statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  if (match(compiler, TOKEN_PRINT)) {
    print_statement(compiler);

  } else if (match(compiler, TOKEN_LEFT_BRACE)) {

    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);

  } else {
    expression_statement(compiler);
  }

  TRACE_PARSER_EXIT();
}

/**/

static void block(Compiler *compiler) {

 // blocks may have multiple declarations, statements including nested blocks
  while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
    declaration(compiler);
  }

  consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

/**/

static void print_statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);
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
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  expression(compiler);
  consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(compiler, OP_POP);
  TRACE_PARSER_EXIT();
}

static void expression(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  /* start with the lowest presedence */
  parse_pressedence(compiler, PREC_ASSIGNMENT);

  TRACE_PARSER_EXIT();
}

/**/

static void parse_pressedence(Compiler *compiler, Presedence presedence) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p, Presedence presedence = %s",
                     compiler, precedence_name(presedence));
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  next_token(compiler);

  TokenType prev_type = compiler->parser.prev.type;
  ParserFunc prefix_rule = get_rule(prev_type)->prefix;

  // the first token should always belong to a prefix expression
  if (prefix_rule == NULL) {
    error(&compiler->parser, "Expected expression.");
    return;
  }

  bool can_assign = presedence <= PREC_ASSIGNMENT;
  prefix_rule(compiler, can_assign);

  // we only parse infix expressions if the current token has a higher
  // presedence than what was passed to parse_pressedence
  while (presedence <= get_rule(compiler->parser.current.type)->presedence) {
    next_token(compiler);

    TokenType prev_type = compiler->parser.prev.type;
    ParserFunc infix_rule = get_rule(prev_type)->infix;

    infix_rule(compiler, can_assign);
    ;
  }

  // variable did not consume = because it's not an assignment
  // report an error
  if (can_assign && match(compiler, TOKEN_EQUAL)) {
    error(&compiler->parser, "Invalid assignment target.");
  }

  TRACE_PARSER_EXIT();
}

static void number(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  double number = strtod(compiler->parser.prev.start, NULL);
  emit_constant(compiler, ant_value.from_number(number));

  TRACE_PARSER_EXIT();
}

/**/

static void grouping(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  expression(compiler);
  consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");

  TRACE_PARSER_EXIT();
}

/**/

static void unary(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  TokenType operator_type = compiler->parser.prev.type;

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

static void binary(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  TokenType operator_type = compiler->parser.prev.type;
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
static void variable(Compiler *compiler, bool can_assign) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);
  named_variable(compiler, compiler->parser.prev, can_assign);
  TRACE_PARSER_EXIT();
}

/* */

static void literal(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  switch (compiler->parser.prev.type) {
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

void string(Compiler *compiler, bool _) {
  const char *chars = compiler->parser.prev.start + 1; // skip the first quote
  int32_t length = compiler->parser.prev.length - 2; // skip the first and last quote
  ObjectString *string = ant_string.make(chars, length);
  Value value = ant_value.from_object(ant_string.as_object(string));

  emit_constant(compiler, value);
}

/* Variables */

static void define_global_variable(Compiler *compiler, int32_t global_index) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  emit_variable(compiler, global_index, ant_chunk.write_define_global);
  TRACE_PARSER_EXIT();
}


static void declare_local_variable(Compiler *compiler){
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

   if(compiler->locals.depth == 0){
     TRACE_PARSER_EXIT();
     return;
   }

   if(compiler->locals.count == OPTION_STACK_MAX){
      error(&compiler->parser, "Too many local variables in scope.");
   }

   Token *token = &compiler->parser.prev;
   bool valid = ant_locals.validate_scope(&compiler->locals, token);

   if(!valid){
      error(&compiler->parser, "Variable with this name already declared in this scope.");
   }

   ant_locals.push(&compiler->locals, *token);
   TRACE_PARSER_EXIT();
}

/* */

static void named_variable(Compiler *compiler, Token name, bool can_assign) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  /* setting up for local or global variables */
  int32_t local    = ant_locals.resolve(&compiler->locals, &name);
  int32_t index    = is_global(local) ? make_global_identifier(compiler, &name) : local;
  Callback get     = is_global(local) ? ant_chunk.write_get_global : ant_chunk.write_get_local;
  Callback set     = is_global(local) ? ant_chunk.write_set_global : ant_chunk.write_set_local;
  
  if (can_assign && match(compiler, TOKEN_EQUAL)) {
    expression(compiler); // on assigment, parse the expression after the equal sign
    emit_variable(compiler, index, set);

  } else {
    emit_variable(compiler, index, get);
  }

  TRACE_PARSER_EXIT();
}

/* */

static int32_t parse_variable(Compiler *compiler, const char *message) {

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  consume(compiler, TOKEN_IDENTIFIER, message);

  // if we are in a block, we are parsing a local variable
  
  declare_local_variable(compiler); // does not emit any instruction
  if(compiler->locals.depth > 0){
    TRACE_PARSER_EXIT();
    return -1; // dummy value
  }

  int32_t globals_index = make_global_identifier(compiler, &compiler->parser.prev);
  TRACE_PARSER_EXIT();
  return globals_index;
}

/* Note that this function stores a constant but doesn't emit
 * emit a constant instruction. So we are returning the index in the
 * constant array so a later instruction can use it.
 * */
static int32_t make_global_identifier(Compiler *compiler, Token *token) {

  ObjectString *str = ant_string.make(token->start, token->length);

  // mapping global variables using the compiler's globals table
  // so vm can access value with O(1) direct indexing
  Value globals_index = ant_mapping.add(&compiler->globals, str);
  return ant_value.as_number(globals_index);
}

/* Block */

static void begin_scope(Compiler *compiler) {
   printf("begin_scope\n");
   compiler->locals.depth++;
}

/* */

static void end_scope(Compiler *compiler){
   printf("end_scope\n");
   compiler->locals.depth--;

   LocalStack *stack = &compiler->locals;

   // clean up locals that are no longer in scope
   // when we reach current stack->depth, we stop
   while(stack->count > 0 && 
         stack->locals[stack->count - 1].depth == stack->depth){
      emit_byte(compiler, OP_POP);
      stack->count--;
   }
}


/* Compilation steps */

static void next_token(Compiler *compiler) {

  compiler->parser.prev = compiler->parser.current;

  while (true) {
    compiler->parser.current = ant_scanner.scan_token(&compiler->scanner);

    if (compiler->parser.current.type != TOKEN_ERROR) {
      break;
    }

    error_at_current(&compiler->parser, compiler->parser.current.start);
  }
}

/* */

static void consume(Compiler *compiler, TokenType type, const char *message) {

  if (compiler->parser.current.type == type) {
    next_token(compiler);
    return;
  }

  error_at_current(&compiler->parser, message);
}

/**/

static void end_of_compilation(Compiler *compiler) {
  emit_byte(compiler, OP_RETURN);

#ifdef DEBUG_PRINT_CODE
  ant_debug.disassemble_chunk(compiler, "code");
#endif
}

/**/

static void synchronize(Compiler *compiler) {
  compiler->parser.panic_mode = false;

  /* syncronize parser to the next statement */
  while (compiler->parser.current.type != TOKEN_EOF) {

    if (compiler->parser.prev.type == TOKEN_SEMICOLON) {
      return;
    }

    switch (compiler->parser.current.type) {
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
  int32_t line = compiler->parser.prev.line;
  bool valid = ant_chunk.write_constant(compiler->current_chunk, value, line);

  if (!valid) {
    error(&compiler->parser, "Too many constants in one chunk.");
  }
}

/**/

static void emit_variable(Compiler *compiler, int32_t index, Callback write_variable) {

  int32_t line = compiler->parser.prev.line;
  bool valid = write_variable(compiler->current_chunk, index, line);

  if (!valid) {
    error(&compiler->parser, "Too many global variables in one chunk.");
  }
}

/**/

static void emit_byte(Compiler *compiler, uint8_t byte) {
  int32_t line = compiler->parser.prev.line;
  ant_chunk.write(compiler->current_chunk, byte, line);
}

/**/

static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2) {
  int32_t line = compiler->parser.prev.line;
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

static ParseRule *get_rule(TokenType type) { return &rules[type]; }

/* */

static bool match(Compiler *compiler, TokenType type) {
  if (!check(compiler, type))
    return false;

  next_token(compiler);
  return true;
}

static bool check(Compiler *compiler, TokenType type) {
  return compiler->parser.current.type == type;
}

static bool is_global(int32_t local_index) {
  return local_index == -1;
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
