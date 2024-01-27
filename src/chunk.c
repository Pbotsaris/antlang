#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {

  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  init_lines(&chunk->lines);
  init_value_array(&chunk->constants);
}

void write_chunk(Chunk *chunk, uint8_t byte, int32_t line) {

  if (chunk->capacity < chunk->count + 1) {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  write_line(&chunk->lines, line, chunk->count);
  chunk->count++;
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  free_lines(&chunk->lines);
  free_value_array(&chunk->constants);
  init_chunk(chunk);
}

int add_constant(Chunk *chunk, Value constant) {
  write_value_array(&chunk->constants, constant);
  return chunk->constants.count - 1;
}
