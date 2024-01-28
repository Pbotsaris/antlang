#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(void) {

  VM *vm = ant_vm.new();

  Chunk chunk;
  ant_chunk.init(&chunk);

  ant_chunk.write_constant(&chunk, 30, 1);
  ant_chunk.write_constant(&chunk, 5, 1);
  ant_chunk.write(&chunk, OP_ADD, 1);
  ant_chunk.write(&chunk, OP_NEGATE, 1);

  ant_chunk.write_constant(&chunk, 2, 1);
  ant_chunk.write(&chunk, OP_DIVIDE, 1);

  ant_chunk.write(&chunk, OP_RETURN, 1);
  ant_vm.interpret(vm, &chunk);

 // InterpretResult res = interpret(&vm, &chunk);

  ant_chunk.free(&chunk);
  ant_vm.free(vm);
  return 0;
}
