#include "vm.h"
#include "common.h"
#include "utils.h"
#include "values.h"
#include "chunk.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

static VM* new_vm();
static void free_vm(VM* vm);
static InterpretResult interpret(VM *vm, Chunk *chunk);
static InterpretResult run(VM *vm);

AntVMAPI ant_vm = {
    .new = new_vm,
    .free = free_vm,
    .interpret = interpret,
};

static VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));

  if (vm == NULL) {
    printf("Error: Could not allocate memory for VM\n");
    exit(1);
  }

  vm->chunk = NULL;
  vm->ip = NULL;
  return vm;
}

static InterpretResult interpret(VM *vm, Chunk *chunk) {
  vm->chunk = chunk;
  vm->ip = vm->chunk->code;
  return run(vm);
}

static void free_vm(VM *vm) {
   ant_chunk.free(vm->chunk);
   free(vm);
}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

  while (true) {

    uint8_t instruction;

    switch ((instruction = READ_BYTE())) {
    case OP_RETURN:
      return INTERPRET_OK;

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      ant_values.print(constant);
      printf("\n");
      break;
    }

    case OP_CONSTANT_LONG: {
      uint8_t *bytes = vm->ip;
      Value constant = ant_utils.unpack_int32(bytes, CONST_24BITS);
      vm->ip += CONST_24BITS;

      ant_values.print(constant);
      printf("\n");
      break;
    }
    }

#undef READ_BYTE
#undef READ_CONSTANT
  }
}
