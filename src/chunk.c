#include <stdlib.h>

#include "chunk.h"
#include "config.h"
#include "memory.h"

static void init_chunk(Chunk *chunk);
static void write_chunk(Chunk *chunk, uint8_t byte, int32_t line);
static void free_chunk(Chunk *chunk);
static bool write_constant(Chunk *chunk, Value value, int32_t line);

AntChunkAPI ant_chunk = {
    .init = init_chunk,
    .write = write_chunk,
    .free = free_chunk,
    .write_constant = write_constant,
};

static void init_chunk(Chunk *chunk) {

  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  ant_line.init(&chunk->lines);
  ant_values.init(&chunk->constants);
}

static void write_chunk(Chunk *chunk, uint8_t byte, int32_t line) {

  if (chunk->capacity < chunk->count + 1) {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  ant_line.write(&chunk->lines, line, chunk->count);
  chunk->count++;
}

static void free_chunk(Chunk *chunk) {
  if (!chunk)
    return;

  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  ant_line.free(&chunk->lines);
  ant_values.free(&chunk->constants);
  init_chunk(chunk);
}

static bool write_constant(Chunk *chunk, Value constant, int32_t line) {

  int32_t constant_index = chunk->constants.count;
  ant_values.write(&chunk->constants, constant);

  /* if we can get away with 8bits, use the more efficient OP_CONSTANT */
  if (constant_index < CONST_MAX_8BITS_VALUE) {
    write_chunk(chunk, OP_CONSTANT, line);
    write_chunk(chunk, constant_index, line);
    return true;
  }

  if (constant_index >= CONST_MAX_24BITS_VALUE) {
    return false;
  }

  /* Otherwise we use OP_CONSTANT_LONG that has a 24 bits operand */
  write_chunk(chunk, OP_CONSTANT_LONG, line);
  write_chunk(chunk, (uint8_t)(constant_index & 0xFF), line);
  write_chunk(chunk, (uint8_t)(constant_index >> 8) & 0xFF, line);
  write_chunk(chunk, (uint8_t)(constant_index >> 16) & 0xFF, line);

  return true;
}
