#ifndef ANT_TOKEN_H
#define ANT_TOKEN_H
#include "common.h"
typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN = 0,
  TOKEN_RIGHT_PAREN = 1,
  TOKEN_LEFT_BRACE = 2,
  TOKEN_RIGHT_BRACE = 3,
  TOKEN_COMMA = 4,
  TOKEN_DOT = 5,
  TOKEN_MINUS = 6,
  TOKEN_PLUS = 7,
  TOKEN_SEMICOLON = 8,
  TOKEN_SLASH = 9,
  TOKEN_STAR = 10,

  // One or two character tokens.
  TOKEN_BANG = 11,
  TOKEN_BANG_EQUAL = 12,
  TOKEN_EQUAL = 13,
  TOKEN_EQUAL_EQUAL = 14,
  TOKEN_GREATER = 15,
  TOKEN_GREATER_EQUAL = 16,
  TOKEN_LESS = 17,
  TOKEN_LESS_EQUAL = 18,

  // Literals.
  TOKEN_IDENTIFIER = 19,
  TOKEN_STRING = 20,
  TOKEN_NUMBER = 21,
  TOKEN_INTERPOLATION_START = 22,
  TOKEN_INTERPOLATION_END = 23,

  // Keywords.
  TOKEN_AND = 24,
  TOKEN_CLASS = 25,
  TOKEN_ELSE = 26,
  TOKEN_FALSE = 27,
  TOKEN_FOR = 28,
  TOKEN_FN = 29,
  TOKEN_IF = 30,
  TOKEN_NIL = 31,
  TOKEN_OR = 32,
  TOKEN_PRINT = 33,
  TOKEN_RETURN = 34,
  TOKEN_SUPER = 35,
  TOKEN_THIS = 36,
  TOKEN_TRUE = 37,
  TOKEN_LET = 38,
  TOKEN_WHILE = 39,

  // Error and end of file tokens.
  TOKEN_ERROR = 40,
  TOKEN_EOF = 41
} TokenType;

typedef struct {
   TokenType type;
   const char *start;
   int32_t length;
   int32_t line;
}Token;


typedef struct {
   const char* (*type_name)(TokenType type);
   void (*print)(Token token);
}TokenAPI;


extern TokenAPI ant_token;

#endif
