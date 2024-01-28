#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(void) {

  VM *vm = ant_vm.new();

  Chunk chunk;
  ant_chunk.init(&chunk);
  ant_chunk.write_constant(&chunk, 30.5, 20);

  for(int i = 0; i < 300; i++){
      ant_chunk.write_constant(&chunk, 20.1+i, 20+1);
  }

  ant_chunk.write(&chunk, OP_RETURN, 10);
  disassemble_chunk(&chunk, "test chunk");

 // InterpretResult res = interpret(&vm, &chunk);

  ant_chunk.free(&chunk);
  ant_vm.free(vm);
  return 0;
}
