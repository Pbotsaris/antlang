#ifndef ANT_CHUNK_H
#define ANT_CHUNK_H
#include "common.h"
#include "lines.h"
#include "values.h"

#define MAX_8BIT_VALUE 256

typedef enum {
  OP_RETURN,        /* no operand */
  OP_CONSTANT,      /* 8-bit operand */
  OP_CONSTANT_LONG, /* 24-bit operand */
} OpCode;

typedef struct {
  int32_t capacity;
  int32_t count;
  ValueArray constants;
  Lines lines;
  uint8_t *code;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int32_t line);
void free_chunk(Chunk *chunk);
void write_constant(Chunk *chunk, Value value, int32_t line);
int add_constant(Chunk *chunk, Value contant);

#endif
