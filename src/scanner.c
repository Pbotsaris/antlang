#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_scanner(Scanner *scanner, const char *source);
static Token scan_token(Scanner *scanner);
static Token error_token(const char *message);

AntScannerAPI ant_scanner = {
    .init = init_scanner,
    .scan_token = scan_token,
};

/* helpers */
static Token string_token(Scanner *scanner);
static Token number_token(Scanner *scanner);
static Token indentifier_token(Scanner *scanner);
static Token make_token(Scanner *scanner, TokenType type);

static TokenType identifier_type(Scanner *scanner);
static TokenType check_keyword(Scanner *scanner, int32_t start, int32_t length,
                               const char *rest, TokenType type);

static void skip_whitespace(Scanner *scanner);
static void skip_comments(Scanner *scanner);
static char eat_char(Scanner *scanner);
static char peek_char(Scanner *scanner);

static bool reached_end(Scanner *scanner);
static bool is_last_char(Scanner *scanner);
static bool match_char(Scanner *scanner, char expected);
static bool is_digit(char c);
static bool is_alpha(char c);

static void init_scanner(Scanner *scanner, const char *source) {
  scanner->start = source;
  scanner->current = source;
  scanner->line = 1;
}

static Token scan_token(Scanner *scanner) {

  skip_whitespace(scanner);
  scanner->start = scanner->current;

  if (reached_end(scanner))
    return (Token){
        .type = TOKEN_EOF, .start = "EOF", .length = 3, .line = scanner->line};

  char c = eat_char(scanner);

  if (is_digit(c)) {
    return number_token(scanner);
  }

  if (is_alpha(c)) {
    return indentifier_token(scanner);
  }

  switch (c) {
  case '(':
    return make_token(scanner, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(scanner, TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(scanner, TOKEN_LEFT_BRACE);
  case '}':
    return make_token(scanner, TOKEN_RIGHT_BRACE);
  case ';':
    return make_token(scanner, TOKEN_SEMICOLON);
  case ',':
    return make_token(scanner, TOKEN_COMMA);
  case '.':
    return make_token(scanner, TOKEN_DOT);
  case '-':
    return make_token(scanner, TOKEN_MINUS);
  case '+':
    return make_token(scanner, TOKEN_PLUS);
  case '/':
    return make_token(scanner, TOKEN_SLASH);
  case '*':
    return make_token(scanner, TOKEN_STAR);

  case '!': {
    TokenType t = match_char(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG;
    return make_token(scanner, t);
  }

  case '=': {
    TokenType t = match_char(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL;
    return make_token(scanner, t);
  }

  case '<': {
    TokenType t = match_char(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS;
    return make_token(scanner, t);
  }

  case '>': {
    TokenType t =
        match_char(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER;
    return make_token(scanner, t);
  }

  case '"':
    return string_token(scanner);
  }

  return error_token("Unexpected character.");
}

/* Helpers */

static Token string_token(Scanner *scanner) {

  while (peek_char(scanner) != '"' && !reached_end(scanner)) {
    if (peek_char(scanner) == '\n') {
      scanner->line++;
    }
    eat_char(scanner);
  }

  if (reached_end(scanner)) {
    return error_token("Unterminated string.");
  }

  eat_char(scanner);
  return make_token(scanner, TOKEN_STRING);
}

static Token number_token(Scanner *scanner) {

  while (is_digit(peek_char(scanner))) {
    eat_char(scanner);
  }

  if (peek_char(scanner) == '.' && is_digit(peek_char(scanner + 1))) {
    eat_char(scanner);

    while (is_digit(peek_char(scanner))) {
      eat_char(scanner);
    }
  }

  return make_token(scanner, TOKEN_NUMBER);
}

static Token indentifier_token(Scanner *scanner) {
  /* accepts both digits and alphas as long as first char is alpha */
  while (is_alpha(peek_char(scanner)) || is_digit(peek_char(scanner))) {
    eat_char(scanner);
  }
  return make_token(scanner, identifier_type(scanner));
}

static Token error_token(const char *message) {
  return (Token){
      .type = TOKEN_ERROR,
      .start = message,
      .length = (int32_t)strlen(message),
      .line = -1,
  };
}

static Token make_token(Scanner *scanner, TokenType type) {
  return (Token){
      .type = type,
      .start = scanner->start,
      .length = (int32_t)(scanner->current - scanner->start),
      .line = scanner->line,
  };
}

static TokenType identifier_type(Scanner *scanner) {
  switch (scanner->start[0]) {
  case 'a':
    return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
  case 'c':
    return check_keyword(scanner, 1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
  case 'i':
    return check_keyword(scanner, 1, 1, "f", TOKEN_IF);
  case 'n':
    return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
  case 'o':
    return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
  case 'p':
    return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return check_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
  case 'l':
    return check_keyword(scanner, 1, 2, "et", TOKEN_LET);
  case 'w':
    return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);

  case 'f':
    if (is_last_char(scanner))
      break;

    switch (scanner->start[1]) {
    case 'n':
      return TOKEN_FN;
    case 'a':
      return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
    case 'o':
      return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
    }

  case 't':
    if (is_last_char(scanner))
      break;

    switch (scanner->start[1]) {
    case 'h':
      return check_keyword(scanner, 2, 2, "is", TOKEN_THIS);

    case 'r':
      return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
    }
  }

  return TOKEN_IDENTIFIER;
}

static TokenType check_keyword(Scanner *scanner, int32_t start, int32_t length,
                               const char *rest, TokenType type) {

  bool has_keyword_length = scanner->current - scanner->start == length + start;

  if (has_keyword_length && memcmp(scanner->start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static void skip_whitespace(Scanner *scanner) {

  while (true) {
    char c = peek_char(scanner);
    bool is_whitespace = c == ' ' || c == '\r' || c == '\t';

    if (is_whitespace) {
      eat_char(scanner);
      continue;
    }

    if (c == '#') {
      skip_comments(scanner);
      continue;
    }

    if (c == '\n') {
      scanner->line++;
      eat_char(scanner);
      continue;
    }

    return;
  }
}

static void skip_comments(Scanner *scanner) {
  while (peek_char(scanner) != '\n' && !reached_end(scanner)) {
    eat_char(scanner);
  }
}

static char eat_char(Scanner *scanner) {
  char c = *(scanner->current);
  scanner->current++;
  return c;
}

static char peek_char(Scanner *scanner) { return *(scanner->current); }
static bool reached_end(Scanner *scanner) { return *scanner->current == '\0'; }
static bool is_last_char(Scanner *scanner) {
  return (scanner->current - scanner->start <= 1);
}

static bool match_char(Scanner *scanner, char expected) {
  if (reached_end(scanner))
    return false;

  if (*(scanner->current) != expected)
    return false;

  scanner->current++;
  return true;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         // accepts underscore
         c == '_';
}
