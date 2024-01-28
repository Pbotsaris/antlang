#include "lines.h"
#include "memory.h"

#include <stdio.h>

static void init_lines(Lines *lines);
static void write_line(Lines *lines, int32_t line, int32_t chunk_index);
static int32_t get_line(Lines *lines, int32_t chunk_index);
static void free_lines(Lines *lines);

AntLineAPI ant_line = {
    .init = init_lines,
    .write = write_line,
    .get = get_line,
    .free = free_lines,
};

/* Helpers */
static bool within_range(int32_t start, int32_t end, int32_t index);
static bool line_exists(Lines *lines, int32_t line_number);

static void init_lines(Lines *lines) {
  lines->count = 0;
  lines->capacity = 0;
  lines->lines = NULL;
}

static void write_line(Lines *lines, int32_t line_number, int32_t chunk_index) {

  if (lines->capacity < lines->count + 1) {
    size_t old_capacity = lines->capacity;
    lines->capacity = GROW_CAPACITY(old_capacity);
    lines->lines =
        GROW_ARRAY(Line, lines->lines, old_capacity, lines->capacity);
  }

  /* NOTE: we optimized adding new lines by not looping through the lines array
   *       This assumes that the bytecode is written in sequential order
   *       If this every changes, this will need to be updated
   * */

  if (line_exists(lines, line_number)) {
    lines->lines[lines->count - 1].end = chunk_index;
    return;
  }

  Line new_line = {
      .start = chunk_index, .end = chunk_index, .number = line_number};

  lines->lines[lines->count] = new_line;
  lines->count++;
}

static int32_t get_line(Lines *lines, int32_t chunk_index) {

  for (int i = 0; i < lines->count; i++) {
    int32_t start = lines->lines[i].start;
    int32_t end = lines->lines[i].end;

    if (within_range(start, end, chunk_index)) {
      return lines->lines[i].number;
    }
  }

  printf("Error: Could not find line for chunk index %d\n", chunk_index);
  return -1;
}

static void free_lines(Lines *lines) {
  if (!lines)
    return;

  FREE_ARRAY(Line, lines->lines, lines->capacity);
  init_lines(lines);
}

static bool line_exists(Lines *lines, int32_t line_number) {
  return lines->count > 0 &&
         lines->lines[lines->count - 1].number == line_number;
}

static bool within_range(int32_t start, int32_t end, int32_t index) {
  return index >= start && index <= end;
}
