#include "token.h"
#include <stdio.h>

static const char *token_type_name(TokenType type);
static void print_token(Token token);

TokenAPI ant_token = {.type_name = token_type_name, .print = print_token};

static void print_token(Token token) {
  printf("[ type: %s", ant_token.type_name(token.type));
  printf("    lexame: %.*s", token.length, token.start);
  printf("    line: %d  ]\n", token.line);
}

static const char *token_type_name(TokenType type) {
  switch (type) {
  case TOKEN_LEFT_PAREN:
    return "TOKEN_LEFT_PAREN";
  case TOKEN_RIGHT_PAREN:
    return "TOKEN_RIGHT_PAREN";
  case TOKEN_LEFT_BRACE:
    return "TOKEN_LEFT_BRACE";
  case TOKEN_RIGHT_BRACE:
    return "TOKEN_RIGHT_BRACE";
  case TOKEN_COMMA:
    return "TOKEN_COMMA";
  case TOKEN_DOT:
    return "TOKEN_DOT";
  case TOKEN_MINUS:
    return "TOKEN_MINUS";
  case TOKEN_PLUS:
    return "TOKEN_PLUS";
  case TOKEN_SEMICOLON:
    return "TOKEN_SEMICOLON";
  case TOKEN_SLASH:
    return "TOKEN_SLASH";
  case TOKEN_STAR:
    return "TOKEN_STAR";
  case TOKEN_BANG:
    return "TOKEN_BANG";
  case TOKEN_BANG_EQUAL:
    return "TOKEN_BANG_EQUAL";
  case TOKEN_EQUAL:
    return "TOKEN_EQUAL";
  case TOKEN_EQUAL_EQUAL:
    return "TOKEN_EQUAL_EQUAL";
  case TOKEN_GREATER:
    return "TOKEN_GREATER";
  case TOKEN_GREATER_EQUAL:
    return "TOKEN_GREATER_EQUAL";
  case TOKEN_LESS:
    return "TOKEN_LESS";
  case TOKEN_LESS_EQUAL:
    return "TOKEN_LESS_EQUAL";
  case TOKEN_IDENTIFIER:
    return "TOKEN_IDENTIFIER";
  case TOKEN_STRING:
    return "TOKEN_STRING";
  case TOKEN_NUMBER:
    return "TOKEN_NUMBER";
  case TOKEN_INTERPOLATION_START:
    return "TOKEN_INTERPOLATION_START";
  case TOKEN_INTERPOLATION_END:
    return "TOKEN_INTERPOLATION_END";
  case TOKEN_AND:
    return "TOKEN_AND";
  case TOKEN_CLASS:
    return "TOKEN_CLASS";
  case TOKEN_ELSE:
    return "TOKEN_ELSE";
  case TOKEN_FALSE:
    return "TOKEN_FALSE";
  case TOKEN_FOR:
    return "TOKEN_FOR";
  case TOKEN_FN:
    return "TOKEN_FN";
  case TOKEN_IF:
    return "TOKEN_IF";
  case TOKEN_NIL:
    return "TOKEN_NIL";
  case TOKEN_OR:
    return "TOKEN_OR";
  case TOKEN_PRINT:
    return "TOKEN_PRINT";
  case TOKEN_RETURN:
    return "TOKEN_RETURN";
  case TOKEN_SUPER:
    return "TOKEN_SUPER";
  case TOKEN_THIS:
    return "TOKEN_THIS";
  case TOKEN_TRUE:
    return "TOKEN_TRUE";
  case TOKEN_LET:
    return "TOKEN_LET";
  case TOKEN_WHILE:
    return "TOKEN_WHILE";
  case TOKEN_ERROR:
    return "TOKEN_ERROR";
  case TOKEN_EOF:
    return "TOKEN_EOF";
  default:
    return "Unknown TokenType";
  }
}
