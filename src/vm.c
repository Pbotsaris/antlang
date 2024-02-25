#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "config.h"
#include "debug.h"
#include "lines.h"
#include "memory.h"
#include "object.h"
#include "strings.h"
#include "value_array.h"
#include "functions.h"
#include "natives.h"
#include "var_mapping.h"
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

/* functions */
static bool call_value(VM *vm, Value callee, int32_t arg_count);
static bool call(VM* vm, ObjectFunction *func, int32_t arg_count);

/* OP Helpers */
static int32_t read_24bit_operand(VM *vm);
static uint16_t read_16bit_operand(VM *vm);
static bool is_numeric_binary_op(VM *vm);
static bool is_string_binary_op(VM *vm);

/* Implementation */
static VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));

  if (vm == NULL) {
    fprintf(stderr, "Error: Could not allocate memory for VM\n");
    exit(1);
  }

  ant_mapping.init();
  ant_stack.reset(&vm->stack);
  ant_compiler.init(&vm->compiler, COMPILATION_TYPE_SCRIPT);
  ant_value_array.init_undefined(&vm->globals);

  ant_native.register_all(vm);
  vm->frame_count = 0;
  return vm;
}

/* */

static InterpretResult interpret(VM *vm, const char *source) {

  ObjectFunction *main_func = ant_compiler.compile(&vm->compiler, source);

  if (main_func == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  /* add main func or type COMPILATION_TYPE_SCRIPT to slot 0 in the stack and calls it
     note that in locals.c:init_local_stack, we claim the slot 0 for the VM for this purpose 
   */
  ant_stack.push(&vm->stack, ant_value.from_object(ant_function.as_object(main_func)));
  call(vm, main_func, 0);

  return run(vm);
}


/* */

static void repl(VM *vm) {
  char line[OPTION_LINE_MAX];

  while (true) {
    printf("ant> ");

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

/* */

static void free_vm(VM *vm) {
  ant_memory.free_objects();

  /* this only clear entries in the hash table
   * the actual strings allocations are managed by the garbage collector
   * */
  ant_string.free_table();
  ant_value_array.free(&vm->globals);
  ant_mapping.free();
  FREE(VM, vm);
}

/* the Massive run function */

static InterpretResult run(VM *vm) {

   CallFrame *frame = vm->frames + (vm->frame_count -1);

#define READ_CHUNK_BYTE() (*frame->ip++)
#define READ_CHUNK_CONSTANT() (frame->func->chunk.constants.values[READ_CHUNK_BYTE()])
#define READ_CHUNK_LONG_CONSTANT()                                             \
  (frame>func->chunk.constants.values[read_24bit_operand(vm)])

#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!is_numeric_binary_op(vm)) {                                           \
      runtime_error(vm, "Operands must be numbers");                           \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = ant_value.as_number(ant_stack.pop(&vm->stack));                 \
    double a = ant_value.as_number(ant_stack.pop(&vm->stack));                  \
    ant_stack.push(&vm->stack, value_type(a op b));                            \
  } while (false)

#ifdef DEBUG_TRACE_EXECUTION
  printf("\n== execution ==\n");
#endif

  for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
    ant_stack.print(&vm->stack);
    /* address to index, get the relative offset */
    int32_t offset = (int32_t)(frame->ip - frame->func->chunk.code);
    ant_debug.disassemble_instruction(&vm->compiler, &frame->func->chunk, offset);
#endif

    uint8_t instruction;

    switch ((instruction = READ_CHUNK_BYTE())) {

    case OP_JUMP: {
      uint16_t offset = read_16bit_operand(vm);
      frame->ip += offset;
      break;
    }

    /* flow control purposefully on top */
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = read_16bit_operand(vm);

      if (ant_value.is_falsey_bool(ant_stack.peek(&vm->stack, 0))) {
        frame->ip += offset;
      }

      break;
    }

    case OP_LOOP: {
      uint16_t offset = read_16bit_operand(vm);
      frame->ip -= offset;
      break;
    }

   /* NOTE: Compiler and vm are setup so that arguments and parameters line up perfectly in the stack 
    *       so there is no need for binding the arguments to the parameters here.
    */
    case OP_CALL: {
      int32_t arg_count = (int32_t)READ_CHUNK_BYTE();
      /* note how arg_count will be the number of arguments on the stack. we grab the last one */
      if(!call_value(vm, ant_stack.peek(&vm->stack, arg_count), arg_count)){
         return INTERPRET_RUNTIME_ERROR;
      }
      /* if call_value is successful there will be a new frame */
      frame = vm->frames + (vm->frame_count - 1);
      break;
   }

    case OP_RETURN:{

         Value result = ant_stack.pop(&vm->stack);;
         vm->frame_count--;
         
      /* script main function */
       if(vm->frame_count == 0){
        ant_stack.pop(&vm->stack);
        return INTERPRET_OK;
       }

       /*  set the stack top to begining of the current frame stack window, 
        *  dicarting stack slots used by function call. Then write return value at stack top.
        *  For example, a sum function with 3 arguments
        *
        *  [script] [4] | [sum] [1] [2] [3] |  
        *               |   stack window    |
        *               ^ move stack top here
        *
        * then push return value
        * [script] [4] [6] 
        *                 ^ stack top
        * */

       vm->stack.top = frame->slots;
       ant_stack.push(&vm->stack, result);
       frame = vm->frames + (vm->frame_count - 1);
       break;
    }

    case OP_NEGATE: {

      if (!ant_value.is_number(ant_stack.peek(&vm->stack, 0))) {
        runtime_error(vm, "Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }

      double num = ant_value.as_number(ant_stack.pop(&vm->stack));
      Value val = ant_value.from_number(num * -1);
      ant_stack.push(&vm->stack, val);
      break;
    }

    case OP_POSITIVE:
      ant_stack.push(&vm->stack, ant_stack.pop(&vm->stack));
      break;

    case OP_ADD: {
      if (is_string_binary_op(vm)) {
        Value b = ant_stack.pop(&vm->stack);
        Value a = ant_stack.pop(&vm->stack);
        ObjectString *str = ant_string.concat(a, b);
        ant_stack.push(&vm->stack, ant_value.from_object(ant_string.as_object(str)));
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
      Value a = ant_stack.pop(&vm->stack);
      Value b = ant_stack.pop(&vm->stack);
      ant_stack.push(&vm->stack, ant_value.equals(b, a));
      break;
    }

    case OP_FALSE:
      ant_stack.push(&vm->stack, ant_value.from_bool(false));
      break;

    case OP_NIL:
      ant_stack.push(&vm->stack, ant_value.make_nil());
      break;

    case OP_NOT: {
      Value value = ant_value.is_falsey(ant_stack.pop(&vm->stack));
      ant_stack.push(&vm->stack, value);
      break;
    }
    case OP_TRUE:
      ant_stack.push(&vm->stack, ant_value.from_bool(true));
      break;

    case OP_PRINT: {
      Value value = ant_stack.pop(&vm->stack);
      ant_value.print(value, false);
      // printf("\n");
      break;
    }

      /* OP_POP discards the top value from the stack */
    case OP_POP: {
      ant_stack.pop(&vm->stack);
      break;
    }

    case OP_GET_LOCAL: {
      int32_t index = (int32_t)READ_CHUNK_BYTE();

      if (ant_stack.is_overflow(&vm->stack, index)) {
        runtime_error(vm, "OP_GET_LOCAL: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }

      // using frame->slots to access relative to the current frame
      ant_stack.push(&vm->stack, frame->slots[index]);
      break;
    }

    case OP_GET_LOCAL_LONG: {
      int32_t index = read_24bit_operand(vm);

      if (ant_stack.is_overflow(&vm->stack, index)) {
        runtime_error(vm, "OP_GET_LOCAL_LONG: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }
      ant_stack.push(&vm->stack, frame->slots[index]);
      break;
    }

    case OP_SET_LOCAL: {
      int32_t index = (int32_t)READ_CHUNK_BYTE();

      if (ant_stack.is_overflow(&vm->stack, index)) {
        runtime_error(vm, "OP_SET_LOCAL: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }
      // using frame->slots to set relative to the current frame
      frame->slots[index] = ant_stack.peek(&vm->stack, 0);
      break;
    }

    case OP_SET_LOCAL_LONG: {
      int32_t index = read_24bit_operand(vm);

      if (ant_stack.is_overflow(&vm->stack, index)) {
        runtime_error(vm, "OP_SET_LOCAL_LONG: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }

      frame->slots[index] = ant_stack.peek(&vm->stack, 0);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      int32_t global_index = (int32_t)READ_CHUNK_BYTE();
      ant_value_array.write_at(&vm->globals, ant_stack.pop(&vm->stack), global_index);
      break;
    }

    case OP_DEFINE_GLOBAL_LONG: {
      int32_t global_index = read_24bit_operand(vm);
      ant_value_array.write_at(&vm->globals, ant_stack.pop(&vm->stack), global_index);
      break;
    }

    case OP_GET_GLOBAL: {
      int32_t global_index = (int32_t)READ_CHUNK_BYTE();
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      ant_stack.push(&vm->stack, value);
      break;
    }

    case OP_GET_GLOBAL_LONG: {
      int32_t global_index = read_24bit_operand(vm);
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      ant_stack.push(&vm->stack, value);
      break;
    }

    case OP_SET_GLOBAL: {
      int32_t global_index = (int32_t)READ_CHUNK_BYTE();
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      // note that we peek the stack here.
      // assigment is an expression, so we need to keep the value on the stack.
      // in case the assignment is part of a larger expression.
      ant_value_array.write_at(&vm->globals, ant_stack.peek(&vm->stack, 0), global_index);
      break;
    }

    case OP_SET_GLOBAL_LONG: {
      int32_t global_index = read_24bit_operand(vm);
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      ant_value_array.write_at(&vm->globals, ant_stack.peek(&vm->stack, 0), global_index);
      break;
    }

    case OP_CONSTANT:
      ant_stack.push(&vm->stack, READ_CHUNK_CONSTANT());
      break;

    case OP_CONSTANT_LONG: {
      double constant = (double)read_24bit_operand(vm);
      ant_stack.push(&vm->stack, ant_value.from_number(constant));
      break;
    }
    }

#undef READ_CHUNK_BYTE
#undef READ_CHUNK_CONSTANT
#undef BINARY_OP
  }
}


static bool call_value(VM *vm, Value callee, int32_t arg_count) {
   if(ant_value.is_object(callee)){
      switch(ant_object.type(callee)){
         case OBJ_FUNCTION:
            return call(vm, ant_function.from_value(callee), arg_count);

         case OBJ_NATIVE:{

            ObjectNative *native = ant_native.from_value(callee);
            Value result         = native->func(arg_count, vm->stack.top - arg_count);

            vm->stack.top -= arg_count + 1;
            ant_stack.push(&vm->stack, result);
            return true;
         }

         default:
            break; // non-callable object
      }
   }

   runtime_error(vm, "Can only call functions and classes");
   return false;
}

/* */

static bool call(VM *vm, ObjectFunction *func, int32_t arg_count) {

   if(arg_count != func->arity){
      runtime_error(vm, "Expected %d arguments but got %d", func->arity, arg_count);
      return false;
   }

   if(vm->frame_count == OPTION_FRAMES_MAX){
      runtime_error(vm, "Reached maximum call stack depth of %d", OPTION_FRAMES_MAX);
      return false;
   }

   /* 
    *  For a code like: 4 + sum(1, 2, 3)
    *  The stack and frame slots would be
    *  [script] [4] | [sum] [1] [2] [3] |
    *               |   frame->slots    |
    */

   CallFrame *frame = vm->frames + vm->frame_count; // next frame
   frame->func = func;
   frame->ip = func->chunk.code;
   // position the slots to be just below the arguments, on function call
   // -1 is to account for stack slot 0 which is reserved for the VM/method calls.
   frame->slots = vm->stack.top - arg_count - 1;
   vm->frame_count++;
   return true;
}

static int32_t read_24bit_operand(VM *vm) {
  CallFrame *frame = vm->frames + vm->frame_count -1;
  frame->ip += CONST_24BITS;
  return (frame->ip[-3] << 16) | (frame->ip[-2] << 8) | (frame->ip[-1]);
}

/**/
static uint16_t read_16bit_operand(VM *vm) {
   CallFrame *frame = vm->frames + vm->frame_count -1;

  frame->ip += CONST_16BITS;
  return (uint16_t)(frame->ip[-2] << 8 | frame->ip[-1]);
}

/**/

static void runtime_error(VM *vm, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for(int32_t i = vm->frame_count -1; i >=0; i--){

    CallFrame *frame = &vm->frames[i];

    ObjectFunction *func = frame->func;
    /* -1 because interpreter is one step ahead when error occurs */
    size_t instruction   = frame->ip - frame->func->chunk.code - 1;  
    int32_t line         = ant_line.get(&frame->func->chunk.lines, instruction);

    fprintf(stderr, "[line %d] in ", line);
   
    if(func->name == NULL){
       fprintf(stderr, "script\n");

    } else {
       fprintf(stderr, "%s()\n", func->name->chars);
    }

  }

  ant_stack.reset(&vm->stack);
}

/**/

static bool is_numeric_binary_op(VM *vm) {
  return ant_value.is_number(ant_stack.peek(&vm->stack, 0)) &&
         ant_value.is_number(ant_stack.peek(&vm->stack, 1));
}

/**/

static bool is_string_binary_op(VM *vm) {
  return ant_object.is_string(ant_stack.peek(&vm->stack, 0)) &&
         ant_object.is_string(ant_stack.peek(&vm->stack, 1));
}
