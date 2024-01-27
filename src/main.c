#include "chunk.h"
#include "debug.h"

int main(void) {
  Chunk chunk;

  init_chunk(&chunk);
  write_chunk(&chunk, OP_RETURN, 10);
  write_chunk(&chunk, OP_RETURN, 10);
  write_chunk(&chunk, OP_RETURN, 10);

  write_constant(&chunk, 30.5, 20);
  write_constant(&chunk, 23.5, 20);

  write_chunk(&chunk, OP_RETURN, 80);
  write_chunk(&chunk, OP_RETURN, 80);
  write_chunk(&chunk, OP_RETURN, 80);

 for(int i = 0; i < 260; i++) {
    write_constant(&chunk, 40.1+i, 81+i);
  }

  disassemble_chunk(&chunk, "test chunk");
  free_chunk(&chunk);

  return 0;
}
