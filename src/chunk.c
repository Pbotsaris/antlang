#include <stdlib.h>

#include "chunk.h"
#include "config.h"
#include "memory.h"

typedef struct {
  OpCode op_8bit;
  OpCode op_24bit;
  int32_t const_index;
  int32_t line;
} WithOperandArgs;

/* Foward declarations */
static void init_chunk(Chunk *chunk);
static void write_chunk(Chunk *chunk, uint8_t byte, int32_t line);
static void free_chunk(Chunk *chunk);
static int32_t add_constant(Chunk *chunk, Value value);
static bool write_constant(Chunk *chunk, Value value, int32_t line);
static bool write_define_global(Chunk *chunk, int32_t const_index,
                                int32_t line);
static bool write_get_global(Chunk *chunk, int32_t const_index, int32_t line);

/* API */
AntChunkAPI ant_chunk = {
    .init = init_chunk,
    .write = write_chunk,
    .free = free_chunk,
    .add_constant = add_constant,
    .write_constant = write_constant,
    .write_define_global = write_define_global,
    .write_get_global = write_get_global,
};

/* Private */
static bool write_chunk_with_operand(Chunk *chunk, WithOperandArgs args);

/* Implementation */

static void init_chunk(Chunk *chunk) {

  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  ant_line.init(&chunk->lines);
  ant_value_array.init(&chunk->constants);
}

/* */

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

/* */

static void free_chunk(Chunk *chunk) {
  if (!chunk)
    return;

  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  ant_line.free(&chunk->lines);
  ant_value_array.free(&chunk->constants);
  init_chunk(chunk);
}

/* */

static bool write_constant(Chunk *chunk, Value constant, int32_t line) {

  int32_t constant_index = chunk->constants.count;
  ant_value_array.write(&chunk->constants, constant);

  WithOperandArgs args = {
      .op_8bit = OP_CONSTANT,
      .op_24bit = OP_CONSTANT_LONG,
      .line = line,
      .const_index = constant_index,
  };

  return write_chunk_with_operand(chunk, args);
}

/* */

static int32_t add_constant(Chunk *chunk, Value constant) {
  int32_t constant_index = chunk->constants.count;
  ant_value_array.write(&chunk->constants, constant);

  return constant_index;
}

/* */

static bool write_define_global(Chunk *chunk, int32_t const_index, int32_t line) {
  WithOperandArgs args = {
      .op_8bit = OP_DEFINE_GLOBAL,
      .op_24bit = OP_DEFINE_GLOBAL_LONG,
      .const_index = const_index,
      .line = line,
  };

  return write_chunk_with_operand(chunk, args);
}

static bool write_get_global(Chunk *chunk, int32_t const_index, int32_t line) {
  WithOperandArgs args = {
      .op_8bit = OP_GET_GLOBAL,
      .op_24bit = OP_GET_GLOBAL_LONG,
      .const_index = const_index,
      .line = line,
  };

  return write_chunk_with_operand(chunk, args);
}

/* Private */

static bool write_chunk_with_operand(Chunk *chunk, WithOperandArgs args) {

  /* if we can get away with 8bits, use the more efficient OP_CONSTANT */
  if (args.const_index < CONST_MAX_8BITS_VALUE) {
    write_chunk(chunk, args.op_8bit, args.line);
    write_chunk(chunk, args.const_index, args.line);
    return true;
  }

  /* Otherwise we use OP_CONSTANT_LONG that has a 24 bits operand */
  if (args.const_index >= CONST_MAX_24BITS_VALUE) {
    return false;
  }

  /* Otherwise we use OP_CONSTANT_LONG that has a 24 bits operand */
  write_chunk(chunk, args.op_24bit, args.line);
  write_chunk(chunk, (uint8_t)(args.const_index & 0xFF), args.line);
  write_chunk(chunk, (uint8_t)(args.const_index >> 8) & 0xFF, args.line);
  write_chunk(chunk, (uint8_t)(args.const_index >> 16) & 0xFF, args.line);

  return true;
}
