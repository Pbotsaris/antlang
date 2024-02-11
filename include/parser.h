#ifndef ANT_PARSER_H
#define ANT_PARSER_H

#include "common.h"
#include "token.h"

typedef struct {
  Token current;
  Token prev;
  bool was_error;
  bool panic_mode;
} Parser;


typedef struct {
   void (*init)(Parser* parser);
   void (*reset)(Parser* parser);
}ParserAPI;

const extern ParserAPI ant_parser;

#endif
