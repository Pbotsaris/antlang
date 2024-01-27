#include <stdio.h>

#include "chunk.h"
#include "debug.h"

int main(void) {

  Chunk chunk;

  init_chunk(&chunk);
  write_chunk(&chunk, OP_RETURN, 10);
  write_chunk(&chunk, OP_RETURN, 10);
  write_chunk(&chunk, OP_RETURN, 10);

  uint8_t const_index = add_constant(&chunk, 2.2);
  write_chunk(&chunk, OP_CONSTANT, 20);
  write_chunk(&chunk, const_index, 20);

  write_chunk(&chunk, OP_RETURN, 80);
  write_chunk(&chunk, OP_RETURN, 80);
  write_chunk(&chunk, OP_RETURN, 80);

  printf("Chunk count: %d\n", chunk.count);
  printf("Chunk capacity: %d\n", chunk.capacity);

  disassemble_chunk(&chunk, "test chunk");
  free_chunk(&chunk);

  return 0;
}
