#include <stdlib.h>

#include "chunk.h"
#include "config.h"
#include "memory.h"

typedef struct {
  OpCode op_8bit;
  OpCode op_24bit;
  int32_t index;
  int32_t line;
} WithOperandArgs;

/* Foward declarations */
static void init_chunk(Chunk *chunk);
static void write_chunk(Chunk *chunk, uint8_t byte, int32_t line);
static bool patch_chunk_16(Chunk *chunk, int32_t offset, int32_t value);

static void free_chunk(Chunk *chunk);
static int32_t add_constant(Chunk *chunk, Value value);
static bool write_constant(Chunk *chunk, Value value, int32_t line);

static bool write_define_global(Chunk *chunk, int32_t global_index, int32_t line);
static bool write_get_global(Chunk *chunk, int32_t global_index, int32_t line);
static bool write_set_global(Chunk *chunk, int32_t global_index, int32_t line);
static bool write_set_local(Chunk *chunk, int32_t local_index, int32_t line);
static bool write_get_local(Chunk *chunk, int32_t local_index, int32_t line);

/* API */
AntChunkAPI ant_chunk = {
    .write = write_chunk,
    .init = init_chunk,
    .patch_16bits = patch_chunk_16,
    .free = free_chunk,
    .add_constant = add_constant,
    .write_constant = write_constant,
    .write_define_global = write_define_global,
    .write_get_global = write_get_global,
    .write_set_global = write_set_global,
    .write_set_local = write_set_local,
    .write_get_local = write_get_local,
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

static bool patch_chunk_16(Chunk *chunk, int32_t offset, int32_t value){

   if(offset + 2 > chunk->count){
      return false;
   }

   if(value > CONST_MAX_16BITS_VALUE){
       return false;
   }

   chunk->code[offset] = (value >> 8) & 0xFF;
   chunk->code[offset + 1] = value & 0xFF;

   return true;
}

/* */

static bool write_constant(Chunk *chunk, Value constant, int32_t line) {

  int32_t constant_index = chunk->constants.count;
  ant_value_array.write(&chunk->constants, constant);

  WithOperandArgs args = {
      .op_8bit = OP_CONSTANT,
      .op_24bit = OP_CONSTANT_LONG,
      .line = line,
      .index = constant_index,
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

static bool write_define_global(Chunk *chunk, int32_t global_index, int32_t line) {
  WithOperandArgs args = {
      .op_8bit = OP_DEFINE_GLOBAL,
      .op_24bit = OP_DEFINE_GLOBAL_LONG,
      .index = global_index,
      .line = line,
  };

  return write_chunk_with_operand(chunk, args);
}

static bool write_get_global(Chunk *chunk, int32_t global_index, int32_t line) {
  WithOperandArgs args = {
      .op_8bit = OP_GET_GLOBAL,
      .op_24bit = OP_GET_GLOBAL_LONG,
      .index = global_index,
      .line = line,
  };

  return write_chunk_with_operand(chunk, args);
}


static bool write_set_global(Chunk *chunk, int32_t global_index, int32_t line) {
  WithOperandArgs args = {
      .op_8bit = OP_SET_GLOBAL,
      .op_24bit = OP_SET_GLOBAL_LONG,
      .index = global_index,
      .line = line,
  };

  return write_chunk_with_operand(chunk, args);
}

static bool write_set_local(Chunk *chunk, int32_t local_index, int32_t line){

   WithOperandArgs args = {
      .op_8bit = OP_SET_LOCAL,
      .op_24bit = OP_SET_LOCAL_LONG,
      .index = local_index,
      .line = line,
   };

   return write_chunk_with_operand(chunk, args);
}


static bool write_get_local(Chunk *chunk, int32_t local_index, int32_t line){
   WithOperandArgs args = {
      .op_8bit = OP_GET_LOCAL,
      .op_24bit = OP_GET_LOCAL_LONG,
      .index = local_index,
      .line = line,
   };

   return write_chunk_with_operand(chunk, args);
}



/* Private */

static bool write_chunk_with_operand(Chunk *chunk, WithOperandArgs args) {

  /* if we can get away with 8bits, use the more efficient OP_CONSTANT */
  if (args.index < CONST_MAX_8BITS_VALUE) {
    write_chunk(chunk, args.op_8bit, args.line);
    write_chunk(chunk, args.index, args.line);
    return true;
  }

  /* Otherwise we use OP_CONSTANT_LONG that has a 24 bits operand */
  if (args.index >= CONST_MAX_24BITS_VALUE) {
    return false;
  }

  /* Otherwise we use OP_CONSTANT_LONG that has a 24 bits operand */
  write_chunk(chunk, args.op_24bit, args.line);
  write_chunk(chunk, (uint8_t)(args.index & 0xFF), args.line);
  write_chunk(chunk, (uint8_t)(args.index >> 8) & 0xFF, args.line);
  write_chunk(chunk, (uint8_t)(args.index >> 16) & 0xFF, args.line);

  return true;
}
