#ifndef ANT_SCANNER_H
#define ANT_SCANNER_H

#include "common.h"
typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  TOKEN_INTERPOLATION_START, TOKEN_INTERPOLATION_END,
  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
   TokenType type;
   const char *start;
   int32_t length;
   int32_t line;
}Token;

typedef struct {
   const char* start;
   const char* current;
   int32_t line;
}Scanner;

typedef struct AntScanner{
   Scanner* (*new)(void);
   void (*init)(Scanner *scanner, const char* source);
   void (*free)(Scanner* scanner);
   Token (*scan_token)(Scanner* scanner);
}AntScannerAPI;


extern AntScannerAPI ant_scanner;

#endif
