#ifndef ANT_LINES_H
#define ANT_LINES_H

#include "common.h"

typedef struct {
  int32_t start;
  int32_t end;
  int32_t number;

} Line;

typedef struct {
  int32_t capacity;
  int32_t count;
  Line *lines;
} Lines;

void init_lines(Lines *lines);
void write_line(Lines *lines, int32_t line, int32_t chunk_index);
int32_t get_line(Lines *lines, int32_t chunk_index);
void free_lines(Lines *lines);

#endif // ANT_LINES_H
