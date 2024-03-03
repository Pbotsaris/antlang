#include "compiler.h"
#include "config.h"
#include "functions.h"
#include "strings.h"
#include "var_mapping.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

typedef enum {
   VAR_RESOLVES_GLOBAL,
   VAR_RESOLVES_LOCAL,
   VAR_RESOLVES_UPVALUE,
   VAR_RESOLVES_ERROR,
}VarResolution;

/* Public */
static void init_compiler(Compiler *compiler, CompilationType type);
static ObjectFunction *compile(Compiler *compiler, const char *source);

const AntCompilerAPI ant_compiler = {
    .init = init_compiler,
    .compile = compile,
};

/** declarations **/
static void declaration(Compiler *compiler);
static void function_declaration(Compiler *compiler);
static void variable_declaration(Compiler *compiler);

/* statements */
static void statement(Compiler *compiler);
static void while_statement(Compiler *compiler);
static void for_statement(Compiler *compiler);
static void print_statement(Compiler *compiler);
static void if_statement(Compiler *compiler);
static void return_statement(Compiler *compiler);
static void expression_statement(Compiler *compiler);
static void block(Compiler *compiler);

/* expression */
static void expression(Compiler *compiler);

/* functions */
static void compile_function(Compiler *parent_compiler, CompilationType type);
static void parse_function_indentifier_and_params(Compiler *compiler);

/* rules */
static void number(Compiler *compiler, bool can_assign);
static void grouping(Compiler *compiler, bool can_assign);
static void unary(Compiler *compiler, bool can_assign);
static void binary(Compiler *compiler, bool can_assign);
static void variable(Compiler *compiler, bool can_assign);
static void call(Compiler *compiler, bool can_assign);
static void literal(Compiler *compiler, bool can_assign);
static void string(Compiler *compiler, bool can_assign);
static void and_operator(Compiler *compiler, bool can_assign);
static void or_operator(Compiler *compiler, bool can_assign);

typedef void (*ParserFunc)(Compiler *, bool can_assign);

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
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
    [TOKEN_AND] = {NULL, and_operator, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_operator, PREC_OR},
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
static void define_local_variable(Compiler *compiler);
static void declare_local_variable(Compiler *compiler);
static void named_variable(Compiler *compiler, Token name, bool can_assign);
static VarResolution resolve_variable_scope(Compiler *compiler, Token *name, int32_t *var_index);
static int32_t resolve_upvalue(Compiler *compiler, Token *name);
static int32_t parse_variable(Compiler *compiler, const char *message);
static int32_t make_global_identifier(Compiler *compiler, Token *token);


/* Functions */
static uint8_t argument_list(Compiler *compiler);

/* Scope */
static void begin_scope(Compiler *compiler);
static void end_scope(Compiler *compiler);

/* Compiling steps */
static void next_token(Compiler *compiler);
static void consume(Compiler *compiler, TokenType type, const char *message);
static ObjectFunction *end_of_compilation(Compiler *compiler);
static void synchronize(Compiler *compiler);
static void patch_jump(Compiler *compiler, int32_t offset);

/* Emitting */
static void emit_constant(Compiler *compiler, Value value);
static void emit_closure(Compiler *compiler, Compiler *func_compiler, ObjectFunction *func);
static void emit_variable(Compiler *compiler, int32_t index, Callback callback);
static int32_t emit_jump(Compiler *compiler, uint8_t instruction);
static void emit_loop(Compiler *compiler, int32_t loop_start);
static void emit_return_nil(Compiler *compiler);
static void emit_byte(Compiler *compiler, uint8_t byte);
static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2);

/* error handling */
static void error_locals_not_initialized(Compiler *compiler);
static int32_t report_on_error(Compiler *compiler, int32_t type);
static void error_at(Parser *parser, const char *message);
static void error(Parser *parser, const char *message);
static void errorf(Parser *parser, const char *format, ...);
static void error_at_current(Parser *parser, const char *message);

/* utils */
static Chunk *current_chunk(Compiler *compiler);
static const char *precedence_name(Presedence presedence);
static bool match(Compiler *compiler, TokenType type);
static bool check(Compiler *compiler, TokenType type);
static ParseRule *get_rule(TokenType type);
static bool is_global(int32_t local_index);
static bool was_local_found(int32_t local_index);

/* Compiler API */
static void init_compiler(Compiler *compiler, CompilationType type) {
  compiler->func      = NULL;
  compiler->enclosing = NULL;
  compiler->type      = type;

  /* scanner gets initialize on compile method */
  ant_parser.init(&compiler->parser);
  ant_locals.init(&compiler->locals);
  compiler->func = ant_function.new();
  ant_upvalues.init(&compiler->upvalues, compiler->func);
}

/**/

static ObjectFunction *compile(Compiler *compiler, const char *source) {
  ant_scanner.init(&compiler->scanner, source);
  ant_parser.reset(&compiler->parser);

#ifdef DEBUG_TRACE_PARSER
  printf("\n== Parser Trace== \n");
#endif

  next_token(compiler);

  while (!match(compiler, TOKEN_EOF)) {
    declaration(compiler);
  }

  ObjectFunction *func = end_of_compilation(compiler);
  return compiler->parser.was_error ? NULL : func;
}

/**/

static void declaration(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  if (match(compiler, TOKEN_FN)) {
    function_declaration(compiler);

  } else if (match(compiler, TOKEN_LET)) {
    variable_declaration(compiler);

  } else {
    statement(compiler);
  }

  if (compiler->parser.panic_mode) {
    synchronize(compiler); // to next statement
  }

  TRACE_PARSER_EXIT();
}

/*
 * Function declarations will work just like variable declarations.
 * When on top level, they will be global variables.
 * within a scope, they will be local variables.
 * */

static void function_declaration(Compiler *compiler) {
  uint8_t global_index = parse_variable(compiler, "Expected function name.");

  ScopeType scope = ant_locals.current_scope(&compiler->locals);

  // for function declarations, we can define locals before the body.
  // this is useful to allow for recursive calls
  if (scope == SCOPE_LOCAL) {
    define_local_variable(compiler);
  }

  compile_function(compiler, COMPILATION_TYPE_FUNC);

  if (scope == SCOPE_GLOBAL) {
    define_global_variable(compiler, global_index);
  }

  if (scope == SCOPE_INVALID) {
    error(&compiler->parser, "Invalid scope for function declaration.");
  }
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

  consume(compiler, TOKEN_SEMICOLON,
          "Expected ';' after variable declaration.");

  ScopeType scope = ant_locals.current_scope(&compiler->locals);

  if (scope == SCOPE_LOCAL) {
    define_local_variable(compiler);
    TRACE_PARSER_EXIT();
    return;
  }

  if (scope == SCOPE_GLOBAL) {
    define_global_variable(compiler, globals_index); // emmits instruction
  } else {
    error(&compiler->parser, "Invalid scope for variable declaration.");
  }
  TRACE_PARSER_EXIT();
}

/**/

static void statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  if (match(compiler, TOKEN_PRINT)) {
    print_statement(compiler);

  } else if (match(compiler, TOKEN_WHILE)) {
    while_statement(compiler);

  } else if (match(compiler, TOKEN_FOR)) {
    for_statement(compiler);

  } else if (match(compiler, TOKEN_LEFT_BRACE)) {
    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);

  } else if (match(compiler, TOKEN_IF)) {
    if_statement(compiler);

   } else if (match(compiler, TOKEN_RETURN)) { 
    return_statement(compiler);

  } else {
    expression_statement(compiler);
  }

  TRACE_PARSER_EXIT();
}

/**/

static void block(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  // blocks may have multiple declarations, statements including nested blocks
  while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
    declaration(compiler);
  }

  consume(compiler, TOKEN_RIGHT_BRACE, "Expected '}' after block.");
  TRACE_PARSER_EXIT();
}

/*  <While Statement (condition)> <--|
 *   |--OP_JUMP_IF_FALSE             |
 *   |  OP_POP                       |
 *   |  <Body statement>             |
 *   |  OP_LOOP                  ----|
 *   |-> jump here
 *
 *      OP_POP
 *      continues...
 *
 * */

static void while_statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  int32_t loop_start = current_chunk(compiler)->count;

  consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
  expression(compiler); // conditionals
  consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after while condition.");

  int32_t exit_jump = emit_jump(
      compiler, OP_JUMP_IF_FALSE); // jump end of loop if condition is false
  emit_byte( compiler, OP_POP); // otherwise we pop the stack and continue with next interation
  statement(compiler); // body
  emit_loop(compiler, loop_start);

  patch_jump(compiler, exit_jump);
  emit_byte(compiler, OP_POP);

  TRACE_PARSER_EXIT();
}

/*  note that we need to run the body before we increment
 *  so we must do some jumping acrobatics
 *
 *       <init clause>
 *        loop_start      <------|
 *       <condition expression>  |
 *      |- OP_JUMP_IF_FALSE      |
 *      |  OP_POP                |
 *  | --|--OP_JUMP               |
 *  |   |  increment_start <-----|---|
 *  |   | <increment expression> |   |
 *  |   |  OP_POP                |   |
 *  |   |  OP_LOOP          -----|   |
 *  |---|--> here..                  |
 *      | <body statement>           |
 *      |                            |
 *      |  OP_LOOP         ----------|
 *      |--> exit jump...
 *         OP_POP
 *      continues...
 *
 * */
static void for_statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  begin_scope(compiler); // for variables are scoped to the loop
  consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

  if (match(compiler, TOKEN_LET)) {
    variable_declaration(compiler);

  } else if (!match(compiler, TOKEN_SEMICOLON)) {
    expression_statement(compiler); // expression statement to consume ; and pop
                                    // value from stack for us
  }

  int32_t loop_start = current_chunk(compiler)->count;
  int32_t exit_jump = -1;

  // conditional expression
  if (!match(compiler, TOKEN_SEMICOLON)) {
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after loop conditions");

    exit_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);
  }

  // increment expression, if any
  if (!match(compiler, TOKEN_RIGHT_PAREN)) {
    int32_t body_jump = emit_jump(compiler, OP_JUMP);
    int32_t increment_start = current_chunk(compiler)->count;
    expression(compiler);
    emit_byte(compiler, OP_POP);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after for loop condition");

    emit_loop(compiler, loop_start);
    loop_start = increment_start;
    patch_jump(compiler, body_jump);
  }

  statement(compiler);
  emit_loop(compiler, loop_start);

  if (exit_jump != -1) {
    patch_jump(compiler, exit_jump);
    emit_byte(compiler, OP_POP);
  }

  end_scope(compiler);
  TRACE_PARSER_EXIT();
}

/*
 *   <Conditional Expression>
 *
 *  |-OP_JUMP_IF_FALSE
 *  |
 *  |    OP_POP
 *  |    <Then Branch Statement>
 *  | |- OP_JUMP
 *  | |
 *  |--> jump here
 *    |  OP_POP
 *    |  <Else Branch Statement>
 *    |
 *    |-> continues...
 *
 * */

static void if_statement(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  consume(compiler, TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
  expression(compiler);
  consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  // this is the point where we from if the condition is false..
  int32_t then_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);

  // if the condition true then...
  emit_byte(compiler, OP_POP);
  statement(compiler);

  // this is the point where we either jump or not the else statement
  int32_t else_jump = emit_jump(compiler, OP_JUMP);

  // if else then...
  patch_jump(compiler, then_jump);
  // this pop is executed if we jump the then statement, otherwise it's ignored
  emit_byte(compiler, OP_POP);

  // if there's an else, compile the statement within
  if (match(compiler, TOKEN_ELSE)) {
    statement(compiler);
  }

  // note that if we don't compile an else statement
  // then the jump will be patched to only skip the
  // last pop instruction
  patch_jump(compiler, else_jump);
  TRACE_PARSER_EXIT();
}

/**/

static void return_statement(Compiler *compiler){

   if(compiler->type == COMPILATION_TYPE_SCRIPT){
      error(&compiler->parser, "Cannot return from top-level code.");
      return;
   }

   /* early return, like the below ;) */
   if(match(compiler, TOKEN_SEMICOLON)){
      emit_return_nil(compiler);
      return; 
   }
   expression(compiler);
   consume(compiler, TOKEN_SEMICOLON, "Expected ';' after return value.");
   emit_byte(compiler, OP_RETURN);
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

/**/

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

static void compile_function(Compiler *parent_compiler, CompilationType type) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", parent_compiler);
  TRACE_PARSER_TOKEN(parent_compiler->parser.prev, parent_compiler->parser.current);

  Compiler func_compiler;
  init_compiler(&func_compiler, type);
  func_compiler.enclosing = parent_compiler;

  // the function compiler will move the parse and scanner along during function compilation
  func_compiler.parser = parent_compiler->parser;
  func_compiler.scanner = parent_compiler->scanner;

  // we just parsed the function name, let's grab it as metadata
  Parser *parser = &parent_compiler->parser;

  if (type != COMPILATION_TYPE_SCRIPT) {
    func_compiler.func->name = ant_string.new(parser->prev.start, parser->prev.length);
  }

  begin_scope(&func_compiler);
  consume(&func_compiler, TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  parse_function_indentifier_and_params(&func_compiler);
  consume(&func_compiler, TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");
  consume(&func_compiler, TOKEN_LEFT_BRACE, "Expected '{' before function body.");
  block(&func_compiler);

  // we do not need to end the scope because the compilation ends here
  ObjectFunction *func = end_of_compilation(&func_compiler);

  // func compiler returns the parser and scanner to the parent compiler
  parent_compiler->parser = func_compiler.parser;
  parent_compiler->scanner = func_compiler.scanner;

  emit_closure(parent_compiler, &func_compiler, func);
  TRACE_PARSER_EXIT();
}

/* */

static void parse_function_indentifier_and_params(Compiler *compiler) {
  if (!check(compiler, TOKEN_RIGHT_PAREN)) {
    do {
      compiler->func->arity++;
      if (compiler->func->arity > OPTION_MAX_NUM_PARAMS) {
        errorf(&compiler->parser, "Cannot have more than %d parameters.", OPTION_MAX_NUM_PARAMS);
      }

      parse_variable(compiler, "Expected parameter name.");
      define_local_variable(compiler);
    } while (match(compiler, TOKEN_COMMA));
  }
}

/* */

static void parse_pressedence(Compiler *compiler, Presedence presedence) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p, Presedence presedence = %s", compiler, precedence_name(presedence));
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

  }

  /* variable did not consume = because it's not an assignment */
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
    break;
  case TOKEN_PLUS:
    emit_byte(compiler, OP_POSITIVE);
    break;
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

static void call(Compiler *compiler, bool can_assign){
   uint8_t arg_count = argument_list(compiler);
   emit_two_bytes(compiler, OP_CALL, arg_count);
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
  int32_t length =
      compiler->parser.prev.length - 2; // skip the first and last quote
  ObjectString *string = ant_string.new(chars, length);
  Value value = ant_value.from_object(ant_string.as_object(string));

  emit_constant(compiler, value);
}

/*
 *   <left operand experession>
 *  |--OP_JUMP_IF_FALSE // if left operand is false, jump to the end
 *  |  OP_POP
 *  |
 *  |<right operand expression>
 *  |-> continues...
 * */

void and_operator(Compiler *compiler, bool _) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  int32_t end_jump = emit_jump(
      compiler,
      OP_JUMP_IF_FALSE); // this is always the point where we jump from

  // if we don't jump, then...
  emit_byte(compiler, OP_POP);
  // compile the right operand
  parse_pressedence(compiler, PREC_AND);

  patch_jump(compiler, end_jump);
  TRACE_PARSER_EXIT();
}

/*
 *    <left operand expression>
 *    |---OP_JUMP_IF_FALSE //
 *  |-|---OP_JUMP
 *  | |-> here..
 *  |     OP_POP
 *  |  <right operand expression>
 *  |-> continues...
 * */

void or_operator(Compiler *compiler, bool _) {
  // if left side operand is false, we skip the next jump
  int else_jump = emit_jump(compiler, OP_JUMP_IF_FALSE);
  // if left side operand is true, skip right side operand
  int end_jump = emit_jump(compiler, OP_JUMP);

  patch_jump(compiler, else_jump);
  emit_byte(compiler, OP_POP);

  parse_pressedence(compiler, PREC_OR);
  patch_jump(compiler, end_jump);
}

/* Variables */

static void define_global_variable(Compiler *compiler, int32_t global_index) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  emit_variable(compiler, global_index, ant_chunk.write_define_global);
  TRACE_PARSER_EXIT();
}

static void define_local_variable(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);
  ant_locals.mark_initialized(&compiler->locals);
  TRACE_PARSER_EXIT();
}

static void declare_local_variable(Compiler *compiler) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  ScopeType scope = ant_locals.current_scope(&compiler->locals);

  if (scope != SCOPE_LOCAL) {
    TRACE_PARSER_EXIT();
    return;
  }

  if (compiler->locals.count == OPTION_STACK_MAX) {
    error(&compiler->parser, "Too many local variables in scope.");
  }

  Token *token = &compiler->parser.prev;
  bool valid = ant_locals.validate_scope(&compiler->locals, token);

  if (!valid) {
    error(&compiler->parser,
          "Variable with this name already declared in this scope.");
  }

  ant_locals.push(&compiler->locals, *token);
  TRACE_PARSER_EXIT();
}

/* */

// TODO: PEdro you must create the write set upvalue and write get upvalue functions :)
static void named_variable(Compiler *compiler, Token name, bool can_assign) {
  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  int32_t var_index = LOCALS_NOT_INTIALIZED;
  VarResolution resolution = resolve_variable_scope(compiler, &name, &var_index);
  Callback get, set;

  switch (resolution) {
     case VAR_RESOLVES_GLOBAL:
        get = ant_chunk.write_get_global;
        set = ant_chunk.write_set_global;
        break;

     case VAR_RESOLVES_LOCAL:
        get = ant_chunk.write_get_local;
        set = ant_chunk.write_set_local;
        break;

      case VAR_RESOLVES_UPVALUE:
        get = ant_chunk.write_get_upvalue;
        set = ant_chunk.write_set_upvalue;
        break;

      case VAR_RESOLVES_ERROR:
        return;

      default:
        error(&compiler->parser, "Invalid variable resolution.");
        return;
  }

  if (can_assign && match(compiler, TOKEN_EQUAL)) {
    expression( compiler); // on assigment, parse the expression after the equal sign
    emit_variable(compiler, var_index, set);

  } else {
    emit_variable(compiler, var_index, get);
  }

  TRACE_PARSER_EXIT();
}
/* */
static VarResolution resolve_variable_scope(Compiler *compiler, Token *name, int32_t *var_index){

  /* attempt to resolve variable in the local scope */
  int32_t local = ant_locals.resolve(&compiler->locals, name);

  if(local == LOCALS_NOT_INTIALIZED){
    error(&compiler->parser, "Cannot read local variable in its own initializer.");
    return VAR_RESOLVES_ERROR;
  }

  if(was_local_found(local)){
    *var_index = local;
    return VAR_RESOLVES_LOCAL;
  }

  int32_t upvalue = resolve_upvalue(compiler, name);

  if(upvalue == UPVALUE_REACHED_MAX || upvalue == UPVALUE_NOT_INITIALIZED){
    return VAR_RESOLVES_ERROR;
  }

  if(was_local_found(upvalue)){
    *var_index = upvalue;
    return VAR_RESOLVES_UPVALUE;
  }

  /* Note the globals resolves at runtime. For now we add it to the mapping. 
   * If it hasn't been declared variable will error at runtime
   * */
  *var_index = make_global_identifier(compiler, name);
  return VAR_RESOLVES_GLOBAL;
}

/* Watch for the recursion  */

int32_t resolve_upvalue(Compiler *compiler, Token *name) {
   // if we are at the top level, we are not in a function
  if (compiler->enclosing == NULL) {
    return LOCALS_NOT_FOUND;
  }

   /* Recursively look for local variable in the enclosing function */
  int32_t local = ant_locals.resolve(&compiler->enclosing->locals, name);

  if(local == LOCALS_NOT_INTIALIZED){
     error_locals_not_initialized(compiler);
     return local;
  }

   // found it
  if(was_local_found(local)){
     return report_on_error(compiler, ant_upvalues.add(&compiler->upvalues, (uint8_t)local, true));
  }

  /* recursively look for upvalues in the enclosing function */
  int32_t upvalue = resolve_upvalue(compiler->enclosing, name);

  /* post order traversal 
   * adds will add the found upvalue to all funcs in the recursive chain 
   * upvalue.is_local = false signifies that upvalue was found in immediate enclosing function
   * */

  if(was_local_found(upvalue)){
     return report_on_error(compiler, ant_upvalues.add(&compiler->upvalues, (uint8_t)upvalue, false));
  }

  return LOCALS_NOT_FOUND;
}

/* */

static int32_t parse_variable(Compiler *compiler, const char *message) {

  TRACE_PARSER_ENTER("Compiler *compiler = %p", compiler);
  TRACE_PARSER_TOKEN(compiler->parser.prev, compiler->parser.current);

  consume(compiler, TOKEN_IDENTIFIER, message);

  // if we are in a block, we are parsing a local variable
  declare_local_variable(compiler); // does not emit any instruction

  ScopeType scope = ant_locals.current_scope(&compiler->locals);

  if (scope == SCOPE_LOCAL) {
    TRACE_PARSER_EXIT();
    return -1; // dummy value
  }

  int32_t globals_index = make_global_identifier(compiler, &compiler->parser.prev);
  TRACE_PARSER_EXIT();
  return globals_index;
}

/* Note that this function creates a global_index but doesn't emit
 * */

static int32_t make_global_identifier(Compiler *compiler, Token *token) {
  ObjectString *str = ant_string.new(token->start, token->length);

  // mapping global variables using the compiler's globals table
  // so vm can access value with O(1) direct indexing
  Value globals_index = ant_mapping.add(str);
  return ant_value.as_number(globals_index);
}

/* Functions */

static uint8_t argument_list(Compiler *compiler){
   uint8_t arg_count = 0;

   if(!check(compiler, TOKEN_RIGHT_PAREN)){

      do {
         expression(compiler);

         if(arg_count == OPTION_MAX_NUM_PARAMS){
             errorf(&compiler->parser, "Cannot have more than %d parameters.", OPTION_MAX_NUM_PARAMS);
         }

         arg_count++;
      } while(match(compiler, TOKEN_COMMA));
   }

  consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
  return arg_count;
}


/* Block */

static void begin_scope(Compiler *compiler) { compiler->locals.depth++; }

/* */

static void end_scope(Compiler *compiler) {
  compiler->locals.depth--;

  LocalStack *stack = &compiler->locals;

  // clean up locals that are no longer in scope
  // when we reach current stack->depth, we stop
  while (stack->count > 0 &&
         stack->locals[stack->count - 1].depth == stack->depth) {
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

ObjectFunction *end_of_compilation(Compiler *compiler) {
  emit_return_nil(compiler);
  ObjectFunction *func = compiler->func;

#ifdef DEBUG_PRINT_CODE
  ant_debug.disassemble_chunk(compiler, &func->chunk, func->name != NULL ? func->name->chars : "<script>");
#endif

  return func;
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

static void patch_jump(Compiler *compiler, int32_t offset) {

  //  calculate the jump distance.
  //  Not that this is not the absolute position we are jumping to
  //  but the offset of the current position
  int32_t jump = current_chunk(compiler)->count - offset - CONST_16BITS;
  bool valid = ant_chunk.patch_16bits(current_chunk(compiler), offset, jump);

  if (!valid) {
    error(&compiler->parser, "Too much code to jump over.");
  }
}

/**/

static void emit_constant(Compiler *compiler, Value value) {
  int32_t line = compiler->parser.prev.line;
  bool valid = ant_chunk.write_constant(current_chunk(compiler), value, line);

  if (!valid) {
    error(&compiler->parser, "Too many constants in one chunk.");
  }
}

/**/

static void emit_closure(Compiler *compiler, Compiler *func_compiler, ObjectFunction *func){
   int32_t line = compiler->parser.prev.line;
   Value value  = ant_value.from_object(ant_function.as_object(func));
   bool valid   = ant_chunk.write_closure(current_chunk(compiler), value, line);

  if (!valid) {
    error(&compiler->parser, "Too many closure constants in one chunk.");
  }

  for(int32_t i = 0; i < func->upvalue_count; i++){
     Upvalue upvalue =  func_compiler->upvalues.values[i];
     ant_chunk.write(current_chunk(compiler), upvalue.is_local ? 1 : 0, line);
     ant_chunk.write(current_chunk(compiler), upvalue.index, line);
  }
}

/**/

static void emit_variable(Compiler *compiler, int32_t index, Callback write_variable) {

  int32_t line = compiler->parser.prev.line;
  bool valid = write_variable(current_chunk(compiler), index, line);

  if (!valid) {
    error(&compiler->parser, "Too many global variables in one chunk.");
  }
}

/**/

static int32_t emit_jump(Compiler *compiler, uint8_t instruction) {
  emit_byte(compiler, instruction);
  /* the nulls re just placeholder for our later 16 bit operand that will patch
   */
  emit_byte(compiler, 0xff);
  emit_byte(compiler, 0xff);
  return current_chunk(compiler)->count - CONST_16BITS;
}

/**/

static void emit_loop(Compiler *compiler, int32_t loop_start) {
  emit_byte(compiler, OP_LOOP);

  // jump the OP_LOOP operands, this is why + CONST_16BITS
  int32_t offset = current_chunk(compiler)->count - loop_start + CONST_16BITS;

  if (offset > CONST_MAX_16BITS_VALUE) {
    error(&compiler->parser, "Loop body too large.");
  }

  emit_two_bytes(compiler, (offset >> 8) & 0xff, offset & 0xff);
}

/**/
static void emit_return_nil(Compiler *compiler) {
  emit_byte(compiler, OP_NIL);
  emit_byte(compiler, OP_RETURN);
}

/**/

static void emit_byte(Compiler *compiler, uint8_t byte) {
  int32_t line = compiler->parser.prev.line;
  ant_chunk.write(current_chunk(compiler), byte, line);
}

/**/

static void emit_two_bytes(Compiler *compiler, uint8_t byte1, uint8_t byte2) {
  int32_t line = compiler->parser.prev.line;
  ant_chunk.write(current_chunk(compiler), byte1, line);
  ant_chunk.write(current_chunk(compiler), byte2, line);
}

/* */

static void error_locals_not_initialized(Compiler *compiler) {
  char *func_name = compiler->func->name != NULL ? compiler->func->name->chars : "<script>";
  errorf(&compiler->parser, "Locals in func '%s' not initlized.", func_name);
}


/* */
int32_t report_on_error(Compiler *compiler, int32_t index) {
   char *func_name = compiler->func->name != NULL ? compiler->func->name->chars : "<script>";

  if (index == UPVALUE_NOT_INITIALIZED) {
    report_on_error(compiler, UPVALUE_NOT_INITIALIZED);
    errorf(&compiler->parser, "Upvalue structure was not initialized in func %s.", func_name);
  }

  if (index == UPVALUE_REACHED_MAX) {
    errorf(&compiler->parser, "Upvalue structure from func %s reached max.", func_name);
  }

  return index;
}



/* */

static void errorf(Parser *parser, const char *format, ...) {
  char message[1024];
  va_list args;
  va_start(args, format);
  vsprintf(message, format, args);
  va_end(args);

  error_at(parser, message);
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

  if (parser->panic_mode) return; // suppress errors after the first one
         
  parser->panic_mode = true;

  fprintf(stderr, "[line %d] Error", token.line);

  switch (token.type) {
  case TOKEN_ERROR:
    // nothing for now
    break;
  case TOKEN_EOF:
    fprintf(stderr, " at end");
    break;
  default:
    fprintf(stderr, " at '%.*s'", token.length, token.start);
    break;
  }

  fprintf(stderr, ": %s\n", message);
  parser->was_error = true;
}
/**/
static Chunk *current_chunk(Compiler *compiler) {
  return &compiler->func->chunk;
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
  return local_index == LOCALS_NOT_FOUND;
}

static bool was_local_found(int32_t local_index) {
  return local_index != LOCALS_NOT_FOUND;
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
