#include "parser.h"


void init_parser(Parser *parser){
   parser->current = (Token){.length = -1, .line = -1, .type = TOKEN_EOF, .start = NULL};
   parser->prev = parser->current;
   parser->was_error = false;
   parser->panic_mode = false;
}


void reset_parser(Parser *parser){
   parser->was_error = false;
   parser->panic_mode = false;
}

const ParserAPI ant_parser = {
   .init = init_parser,
};
