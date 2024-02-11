#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "config.h"
#include "debug.h"
#include "lines.h"
#include "memory.h"
#include "object.h"
#include "strings.h"
#include "utils.h"
#include "value_array.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Public */
static VM *new_vm();
static InterpretResult interpret(VM *vm, const char *source);
static void repl(VM *vm);
static void free_vm(VM *vm);

AntVMAPI ant_vm = {
    .new = new_vm,
    .free = free_vm,
    .interpret = interpret,
    .repl = repl,
};

/* VM */
static InterpretResult run(VM *vm);
static void runtime_error(VM *vm, const char *format, ...);

/* Stack */
static void reset_stack(VM *vm);
static void push_stack(VM *vm, Value value);
static Value pop_stack(VM *vm);
static void print_stack(VM *vm);
static Value peek_stack(VM *vm, int32_t distance);

/* OP Helpeers */
static int32_t read_24bit_operand(VM *vm);
static bool is_numeric_binary_op(VM *vm);
static bool is_string_binary_op(VM *vm);

/* Implementation */
static VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));

  if (vm == NULL) {
    fprintf(stderr, "Error: Could not allocate memory for VM\n");
    exit(1);
  }

  reset_stack(vm);

  vm->chunk = NULL;
  vm->ip = NULL;

  ant_compiler.init(&vm->compiler);
  ant_value_array.init_nils(&vm->globals);

  return vm;
}

static InterpretResult interpret(VM *vm, const char *source) {

  Chunk chunk;
  ant_chunk.init(&chunk);

  bool valid = ant_compiler.compile(&vm->compiler, source, &chunk);

  if (!valid) {
    ant_chunk.free(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm->chunk = &chunk;
  vm->ip = vm->chunk->code;

  InterpretResult result = run(vm);

  ant_chunk.free(&chunk);
  return result;
}

static void repl(VM *vm) {
  char line[OPTION_LINE_MAX];

  while (true) {
    printf("> ");

    if (!fgets(line, OPTION_LINE_MAX, stdin)) {
      printf("\n");
      break;
    }

    if (line[0] == '\n') {
      continue;
    }

    if (strcmp(line, "exit\n") == 0 || strcmp(line, "q\n") == 0) {
      break;
    }

    interpret(vm, line);
    printf("\n");
  }
}

static void free_vm(VM *vm) {
  ant_compiler.free(&vm->compiler);
  ant_memory.free_objects();
  ant_string.free_all();
  ant_value_array.free(&vm->globals);
  free(vm);
}

static InterpretResult run(VM *vm) {
#define READ_CHUNK_BYTE() (*vm->ip++)
#define READ_CHUNK_CONSTANT() (vm->chunk->constants.values[READ_CHUNK_BYTE()])
#define READ_CHUNK_LONG_CONSTANT()                                             \
  (vm->chunk->constants.values[read_24bit_operand(vm)])

#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!is_numeric_binary_op(vm)) {                                           \
      runtime_error(vm, "Operands must be numbers");                           \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = ant_value.as_number(pop_stack(vm));                             \
    double a = ant_value.as_number(pop_stack(vm));                             \
    push_stack(vm, value_type(a op b));                                        \
  } while (false)

#ifdef DEBUG_TRACE_EXECUTION
      printf("\n== execution ==\n");
#endif


    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
      print_stack(vm);

      /* address to index, get the relative offset */
      int32_t offset = (int32_t)(vm->ip - vm->chunk->code);
      ant_debug.disassemble_instruction(vm->chunk, offset);
#endif

      uint8_t instruction;

      switch ((instruction = READ_CHUNK_BYTE())) {

      case OP_RETURN:
        return INTERPRET_OK;

      case OP_NEGATE: {

        if (!ant_value.is_number(peek_stack(vm, 0))) {
          runtime_error(vm, "Operand must be a number");
          return INTERPRET_RUNTIME_ERROR;
        }

        double num = ant_value.as_number(pop_stack(vm));
        Value val = ant_value.from_number(num * -1);
        push_stack(vm, val);
        break;
      }

      case OP_POSITIVE:
        push_stack(vm, pop_stack(vm));
        break;

      case OP_ADD: {
        if (is_string_binary_op(vm)) {
          Value b = pop_stack(vm);
          Value a = pop_stack(vm);
          ObjectString *str = ant_string.concat(a, b);
          push_stack(vm, ant_value.from_object(ant_string.as_object(str)));
          break;
        }

        BINARY_OP(ant_value.from_number, +);
        break;
      }

      case OP_SUBTRACT:
        BINARY_OP(ant_value.from_number, -);
        break;

      case OP_MULTIPLY:
        BINARY_OP(ant_value.from_number, *);
        break;

      case OP_DIVIDE:
        BINARY_OP(ant_value.from_number, /);
        break;

      case OP_GREATER:
        BINARY_OP(ant_value.from_bool, >);
        break;

      case OP_LESS:
        BINARY_OP(ant_value.from_bool, <);
        break;

      case OP_EQUAL: {
        Value a = pop_stack(vm);
        Value b = pop_stack(vm);
        push_stack(vm, ant_value.equals(b, a));
        break;
      }

      case OP_FALSE:
        push_stack(vm, ant_value.from_bool(false));
        break;

      case OP_NIL:
        push_stack(vm, ant_value.make_nil());
        break;

      case OP_NOT: {
        Value value = ant_value.is_falsey(pop_stack(vm));
        push_stack(vm, value);
        break;
      }
      case OP_TRUE:
        push_stack(vm, ant_value.from_bool(true));
        break;

      case OP_PRINT: {
        Value value = pop_stack(vm);
        ant_value.print(value);
        printf("\n");
        break;
      }

        /* OP_POP discards the top value from the stack */
      case OP_POP: {
        pop_stack(vm);
        break;
      }

      case OP_DEFINE_GLOBAL: {
        int32_t global_index = (int32_t)READ_CHUNK_BYTE();
        ant_value_array.write_at(&vm->globals,pop_stack(vm), global_index);
        break;
      }

      case OP_DEFINE_GLOBAL_LONG: {
         int32_t global_index = read_24bit_operand(vm);
         ant_value_array.write_at(&vm->globals,pop_stack(vm), global_index);
        break;
      }

      case OP_GET_GLOBAL: {
        int32_t global_index = (int32_t)READ_CHUNK_BYTE();
        Value value = ant_value_array.at(&vm->globals, global_index);

        if(ant_value.is_nil(value)){
           runtime_error(vm, "Undefined variable");
           return INTERPRET_RUNTIME_ERROR;
        }

        push_stack(vm, value);
        break;
      }

      case OP_GET_GLOBAL_LONG: {
       int32_t global_index = read_24bit_operand(vm);
       Value value          = ant_value_array.at(&vm->globals, global_index);

        if(ant_value.is_nil(value)){
           runtime_error(vm, "Undefined variable");
           return INTERPRET_RUNTIME_ERROR;
        }

        push_stack(vm, value);
        break;
      }

      case OP_SET_GLOBAL: {
       int32_t global_index = (int32_t)READ_CHUNK_BYTE();
       Value value          = ant_value_array.at(&vm->globals, global_index);

       if(ant_value.is_nil(value)){
           runtime_error(vm, "Undefined variable");
           return INTERPRET_RUNTIME_ERROR;
        }

        // note that we peek the stack here.
        // assigment is an expression, so we need to keep the value on the stack.
        // in case the assignment is part of a larger expression.
        ant_value_array.write_at(&vm->globals, peek_stack(vm, 0), global_index);
        break;
      }

     case OP_SET_GLOBAL_LONG: {
       int32_t global_index = read_24bit_operand(vm);
       Value value          = ant_value_array.at(&vm->globals, global_index);

        if(ant_value.is_nil(value)){
           runtime_error(vm, "Undefined variable");
           return INTERPRET_RUNTIME_ERROR;
        }

        ant_value_array.write_at(&vm->globals, peek_stack(vm, 0), global_index);
        break;
      }

      case OP_CONSTANT:
        push_stack(vm, READ_CHUNK_CONSTANT());
        break;

      case OP_CONSTANT_LONG: {
        double constant = (double)read_24bit_operand(vm);
        push_stack(vm, ant_value.from_number(constant));
        break;
      }
      }

#undef READ_CHUNK_BYTE
#undef READ_CHUNK_CONSTANT
#undef BINARY_OP
    }
}

/* Stack */

static void reset_stack(VM *vm) { vm->stack_top = vm->stack; }

/**/

static void push_stack(VM *vm, Value value) {

  int32_t stack_index = (int32_t)(vm->stack_top - vm->stack);

  if (stack_index == OPTION_STACK_MAX) {
    fprintf(stderr, "Stack overflow\n");
    exit(11);
  }

  *(vm->stack_top) = value;
  vm->stack_top++;
}

/**/

static int32_t read_24bit_operand(VM *vm) {
  uint8_t *bytes = vm->ip;
  int32_t index = ant_utils.unpack_int32(bytes, CONST_24BITS);
  vm->ip += CONST_24BITS;
  return index;
}

/**/

static void runtime_error(VM *vm, const char *format, ...) {
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
   fputs("\n", stderr);

   /* -1 because interpreter is one step ahead */
   size_t instruction = vm->ip - vm->chunk->code - 1;
   int32_t line = ant_line.get(&vm->chunk->lines, instruction);

   fprintf(stderr, "[line %d] in script\n", line);
   reset_stack(vm);
}

/**/

static Value pop_stack(VM *vm) {

   if (vm->stack_top == vm->stack) {
      fprintf(stderr, "Stack underflow\n");
      exit(11);
   }

   vm->stack_top--;
   return *(vm->stack_top);
}

static void print_stack(VM *vm) {
   printf("        ");
   for (Value *slot = vm->stack; slot < vm->stack_top; slot++) {
      printf("[");
      ant_value.print(*slot);
      printf("]");
   }
   printf("\n");
}

/**/

static Value peek_stack(VM *vm, int32_t distance) {
   return vm->stack_top[-1 - distance];
}

/**/

static bool is_numeric_binary_op(VM *vm) {
   return ant_value.is_number(peek_stack(vm, 0)) &&
      ant_value.is_number(peek_stack(vm, 1));
}

/**/

static bool is_string_binary_op(VM *vm) {
   return ant_object.is_string(peek_stack(vm, 0)) &&
      ant_object.is_string(peek_stack(vm, 1));
}
