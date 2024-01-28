#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "config.h"
#include "debug.h"
#include "utils.h"
#include "values.h"

#include <stdio.h>
#include <stdlib.h>

/* Public */
static VM *new_vm();
static void free_vm(VM *vm);
static InterpretResult interpret(VM *vm, Chunk *chunk);

AntVMAPI ant_vm = {
    .new = new_vm,
    .free = free_vm,
    .interpret = interpret,
};

/* helpers */
static InterpretResult run(VM *vm);

static void reset_stack(VM *vm);
static void push_stack(VM *vm, Value value);
static Value pop_stack(VM *vm);
static void print_stack(VM *vm);

/* Implementation */
static VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));

  if (vm == NULL) {
    printf("Error: Could not allocate memory for VM\n");
    exit(1);
  }

  reset_stack(vm);
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
 // ant_chunk.free(vm->chunk);
  free(vm);
}

static InterpretResult run(VM *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

#define BINARY_OP(op)                                                          \
  do {                                                                         \
    Value b = pop_stack(vm);                                                  \
    Value a = pop_stack(vm);                                                  \
    push_stack(vm, a op b);                                                    \
  } while (false)

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    print_stack(vm);

    /* address to index, get the relative offset */
    int32_t offset = (int32_t)(vm->ip - vm->chunk->code);
    ant_debug.disassemble_instruction(vm->chunk, offset);
#endif

    uint8_t instruction;

    switch ((instruction = READ_BYTE())) {
    case OP_RETURN:
      ant_values.print(pop_stack(vm));
      return INTERPRET_OK;

    case OP_NEGATE:
      push_stack(vm, pop_stack(vm) * -1);
      break;

    case OP_ADD:
      BINARY_OP(+);
      break;

    case OP_SUBTRACT:
      BINARY_OP(-);
      break;

    case OP_MULTIPLY:
      BINARY_OP(*);
      break;

    case OP_DIVIDE:
      BINARY_OP(/);
      break;

    case OP_CONSTANT:
      push_stack(vm, READ_CONSTANT());
      break;

    case OP_CONSTANT_LONG: {
      uint8_t *bytes = vm->ip;
      Value constant = ant_utils.unpack_int32(bytes, CONST_24BITS);
      vm->ip += CONST_24BITS;

      push_stack(vm, constant);
      break;
    }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
  }
}

static void reset_stack(VM *vm) { vm->stack_top = vm->stack; }

static void push_stack(VM *vm, Value value) {

  int32_t stack_index = (int32_t)(vm->stack_top - vm->stack);

  if (stack_index == CONST_STACK_MAX) {
    printf("Error: Stack overflow\n");
    exit(1);
  }

  *(vm->stack_top) = value;
  vm->stack_top++;
}

static Value pop_stack(VM *vm) {

  if (vm->stack_top == vm->stack) {
    printf("Error: Stack underflow\n");
    exit(1);
  }

  vm->stack_top--;
  return *(vm->stack_top);
}

static void print_stack(VM *vm) {
  printf("        ");
  for (Value *slot = vm->stack; slot < vm->stack_top; slot++) {
    printf("[");
    ant_values.print(*slot);
    printf("]");
  }
  printf("\n");
}
