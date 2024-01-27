#ifndef ANT_CHUNK_H
#define ANT_CHUNK_H
#include "common.h"
#include "values.h"
#include "lines.h"

typedef enum {
   OP_RETURN,
   OP_CONSTANT,
}OpCode;

typedef struct {
   int32_t capacity;
   int32_t count;
   ValueArray constants;
   Lines lines;
   uint8_t* code;
}Chunk;

void init_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte, int32_t line);
void free_chunk(Chunk* chunk);
int add_constant(Chunk* chunk, Value contant);

#endif

// OP_CODES     ->  [OP_1, OP_2, OP_3, OP_4, OP_5, OP_6, OP_7, OP_8, OP_9]
// line number ->   [1,     1,    1,    1,    2,    2,    2,    3,    3]

/* 
 *         start end value...
  lines = [ 0,   3,   1,   4, 6, 2, 7,8, 3]


 */
   
