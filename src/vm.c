#include "vm.h"
#include "chunk.h"
#include "common.h"
#include "config.h"
#include "lines.h"
#include "memory.h"
#include "object.h"
#include "strings.h"
#include "value_array.h"
#include "functions.h"
#include "closure.h"
#include "natives.h"
#include "var_mapping.h"
#include "upvalues.h"
#include "stack.h"

#include "debug.h"
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
static bool call(VM* vm, ObjectClosure *closure, int32_t arg_count);

/* Implementation */
static VM *new_vm() {
  VM *vm = (VM *)malloc(sizeof(VM));

  if (vm == NULL) {
    fprintf(stderr, "Error: Could not allocate memory for VM\n");
    exit(1);
  }

  STACK_RESET();
  ant_mapping.init();

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
  STACK_PUSH(VALUE_FROM_OBJECT(FUNCTION_AS_OBJECT(main_func)));
  STACK_POP(); // push pop silliness for the GC
               
  ObjectClosure *closure = ant_closure.new(main_func);
  STACK_PUSH(VALUE_FROM_OBJECT(CLOSURE_AS_OBJECT(closure)));

  call(vm, closure, 0);
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


    // FIX ME: this is a hack to reset the compiler state. look into a better way to do this
    ant_compiler.init(&vm->compiler, COMPILATION_TYPE_SCRIPT);
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
   register uint8_t *ip = frame->ip;

#define READ_CHUNK_BYTE() (*ip++)
#define READ_CHUNK_CONSTANT() (frame->closure->func->chunk.constants.values[READ_CHUNK_BYTE()])
#define READ_CHUNK_LONG_CONSTANT() (frame->closure->func->chunk.constants.values[READ_24BIT_OPERANDS()])

#define READ_24BIT_OPERANDS() ({                                     \
           ip += CONST_24BITS;                                       \
    (int)((ip[-3] << 16) | (ip[-2] << 8) | ip[-1]);                       \
})

#define READ_16BIT_OPERANDS() ({                                     \
    ip += CONST_16BITS;                                              \
    (uint16_t)((ip[-2] << 8) | ip[-1]);                              \
})

#define IS_NUMERIC_BINARY_OP() (                                     \
    VALUE_IS_NUMBER(STACK_PEEK(0)) &&                                \
    VALUE_IS_NUMBER(STACK_PEEK(1))                                   \
)

#define IS_STRING_BINARY_OP() (                                      \
    OBJECT_IS_STRING(STACK_PEEK(0)) &&                               \
    OBJECT_IS_STRING(STACK_PEEK(1))                                  \
)

#define BINARY_OP(value_type, op)                                    \
  do {                                                               \
    if (!IS_NUMERIC_BINARY_OP()) {                                   \
      runtime_error(vm, "Operands must be numbers");                 \
      return INTERPRET_RUNTIME_ERROR;                                \
    }                                                                \
    double b = VALUE_AS_NUMBER(STACK_POP());                         \
    double a = VALUE_AS_NUMBER(STACK_POP());                         \
    STACK_PUSH(value_type(a op b));                                  \
  } while (false)

#ifdef DEBUG_TRACE_EXECUTION
  printf("\n== execution ==\n");
#endif

  for (;;) {

#ifdef DEBUG_TRACE_EXECUTION
    /* address to index, get the relative offset */
    int32_t offset = (int32_t)(ip - frame->closure->func->chunk.code);
    ant_debug.disassemble_instruction(&vm->compiler, &frame->closure->func->chunk, offset);
    print_stack();
#endif

    uint8_t instruction;

    switch ((instruction = READ_CHUNK_BYTE())) {

    case OP_JUMP: {
      uint16_t offset = READ_16BIT_OPERANDS();
      ip += offset;
      break;
    }

    /* flow control purposefully on top */
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_16BIT_OPERANDS();

      if (VALUE_IS_FALSEY_AS_BOOL(STACK_PEEK(0))) {
        ip += offset;
      }

      break;
    }

    case OP_LOOP: {
      uint16_t offset = READ_16BIT_OPERANDS();
      ip -= offset;
      break;
    }

   /* NOTE: Compiler and vm are setup so that arguments and parameters line up perfectly in the stack 
    *       so there is no need for binding the arguments to the parameters here.
    */
    case OP_CALL: {
      int32_t arg_count = (int32_t)READ_CHUNK_BYTE();
      frame->ip = ip; // sync frame ip with ip
                      
      /* note how arg_count will be the number of arguments on the stack. we grab the last one */
      if(!call_value(vm, STACK_PEEK(arg_count), arg_count)){
         return INTERPRET_RUNTIME_ERROR;
      }
      /* if call_value is successful there will be a new frame */
      frame = vm->frames + (vm->frame_count - 1);
      ip    = frame->ip;
      break;
   }


   case OP_SET_UPVALUE: {
      uint8_t slot = READ_CHUNK_BYTE();
      *frame->closure->upvalues[slot]->location = STACK_PEEK(0);
      break;
   }


   case OP_GET_UPVALUE: {
      /* the operand is the index into the current function's upvalue array */
      uint8_t slot = READ_CHUNK_BYTE();
      STACK_PUSH(*frame->closure->upvalues[slot]->location);
      break;
   }


#define CAPTURE_UPVALUES(closure, frame)                                                                \
                                                                                                        \
    /* the compiler emits closure->upvalue_count number of */                                           \
    /* [is_local, index] pairs to the chunk after constants operand. */                                 \
    /* reading the upvalues from the chunk below */                                                     \
                                                                                                        \
    for (int32_t i = 0; i < closure->upvalue_count; i++) {                                              \
                                                                                                        \
        /* NOTE: OP_CLOSURE is emitted during function declaration, so at this point of the */          \
        /* execution, the CallFrame is at the enclosing function of the function being declared. */     \
                                                                                                        \
        uint8_t is_local = READ_CHUNK_BYTE();                                                           \
        uint8_t index = READ_CHUNK_BYTE();                                                              \
                                                                                                        \
        /* the upvalue closes over a local variable in the enclosing function (the current frame) */    \
        /* we capture the local variable based on the relative index */                                 \
        /* provided by the closure instruction. */                                                      \
                                                                                                        \
        if (is_local) {                                                                                 \
            closure->upvalues[i] = ant_upvalues.capture(&frame->slots[index]);                          \
            continue;                                                                                   \
        }                                                                                               \
        /* Upvalue is not local (to the enclosing function of the function being declared), */          \
        /* This means the upvalue was captured someplace, grab it from the parent closure. */           \
                                                                                                        \
        closure->upvalues[i] = frame->closure->upvalues[index];                                         \
    }

   case OP_CLOSURE: {

      /* closures are like special case constant, the same but with some pre-processing 
       * before pushing it to the stack 
       *
       * Then, we capture the upvalues of the closure
       * */

      ObjectFunction *func = FUNCTION_FROM_VALUE(READ_CHUNK_CONSTANT());
      ObjectClosure *closure = ant_closure.new(func);
      STACK_PUSH(VALUE_FROM_OBJECT(CLOSURE_AS_OBJECT(closure)));
      CAPTURE_UPVALUES(closure, frame);
      break;
   }

  case OP_CLOSURE_LONG: {
      ObjectFunction *func = FUNCTION_FROM_VALUE(READ_CHUNK_LONG_CONSTANT());
      ObjectClosure *closure = ant_closure.new(func);
      STACK_PUSH(VALUE_FROM_OBJECT(CLOSURE_AS_OBJECT(closure)));
      CAPTURE_UPVALUES(closure, frame);
      break;
   }

#undef CAPTURE_UPVALUES

    case OP_RETURN:{
         Value result = STACK_POP();
         vm->frame_count--;
         
      /* script main function */
       if(vm->frame_count == 0){
        STACK_POP();
        ip = frame->ip;
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

       stack.top = frame->slots;
       STACK_PUSH(result);
       frame = vm->frames + (vm->frame_count - 1);
       ip = frame->ip;
       break;
    }

    case OP_NEGATE: {
      if (!VALUE_IS_NUMBER(STACK_PEEK(0))) {
        runtime_error(vm, "Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }

      double num = VALUE_AS_NUMBER(STACK_POP());
      Value val = VALUE_FROM_NUMBER(num * -1);
      STACK_PUSH(val);
      break;
    }

    case OP_POSITIVE:
      STACK_PUSH(STACK_POP());
      break;

    case OP_ADD: {
      if (IS_STRING_BINARY_OP()) {
        Value b = STACK_POP();
        Value a = STACK_POP();
        ObjectString *str = ant_string.concat(a, b);
        STACK_PUSH(VALUE_FROM_OBJECT(STRING_AS_OBJECT(str)));
        break;
      }

      BINARY_OP(VALUE_FROM_NUMBER, +);
      break;
    }

    case OP_SUBTRACT:
      BINARY_OP(VALUE_FROM_NUMBER, -);
      break;

    case OP_MULTIPLY:
      BINARY_OP(VALUE_FROM_NUMBER, *);
      break;

    case OP_DIVIDE:
      BINARY_OP(VALUE_FROM_NUMBER, /);
      break;

    case OP_GREATER:
      BINARY_OP(VALUE_FROM_BOOL, >);
      break;

    case OP_LESS:
      BINARY_OP(VALUE_FROM_BOOL, <);
      break;

    case OP_EQUAL: {
      Value a = STACK_POP();
      Value b = STACK_POP();
      STACK_PUSH(VALUE_EQUALS(b, a));
      break;
    }

    case OP_FALSE:
      STACK_PUSH(VALUE_FROM_BOOL(false));
      break;

    case OP_NIL:
      STACK_PUSH(VALUE_FROM_NIL());
      break;

    case OP_NOT: {
      Value value = VALUE_IS_FALSEY(STACK_POP());
      STACK_PUSH(value);
      break;
    }
    case OP_TRUE:
      STACK_PUSH(VALUE_FROM_BOOL(true));
      break;

    case OP_PRINT: {
      Value value = STACK_POP();
      ant_value.print(value, false);
      printf("\n");
      break;
    }

      /* OP_POP discards the top value from the stack */
    case OP_POP: {
      STACK_POP();
      break;
    }

    case OP_GET_LOCAL: {
      int32_t index = (int32_t)READ_CHUNK_BYTE();

      if (STACK_OVERFLOW(index)) {
        runtime_error(vm, "OP_GET_LOCAL: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }

      // using frame->slots to access relative to the current frame
      STACK_PUSH(frame->slots[index]);
      break;
    }

    case OP_GET_LOCAL_LONG: {
      int32_t index = READ_24BIT_OPERANDS();

      if (STACK_OVERFLOW(index)) {
        runtime_error(vm, "OP_GET_LOCAL_LONG: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }
      STACK_PUSH(frame->slots[index]);
      break;
    }

    case OP_SET_LOCAL: {
      int32_t index = (int32_t)READ_CHUNK_BYTE();

      if (STACK_OVERFLOW(index)) {
        runtime_error(vm, "OP_SET_LOCAL: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }
      // using frame->slots to set relative to the current frame
      frame->slots[index] = STACK_PEEK(0);
      break;
    }

    case OP_SET_LOCAL_LONG: {
      int32_t index = READ_24BIT_OPERANDS();

      if (STACK_OVERFLOW(index)) {
        runtime_error(vm, "OP_SET_LOCAL_LONG: Stack overflow at index %d", index);
        return INTERPRET_RUNTIME_ERROR;
      }

      frame->slots[index] = STACK_PEEK(0);
      break;
    }

    case OP_DEFINE_GLOBAL: {
      int32_t global_index = (int32_t)READ_CHUNK_BYTE();
      ant_value_array.write_at(&vm->globals, STACK_POP(), global_index);
      break;
    }

    case OP_DEFINE_GLOBAL_LONG: {
      int32_t global_index = READ_24BIT_OPERANDS();
      ant_value_array.write_at(&vm->globals, STACK_POP(), global_index);
      break;
    }

    case OP_GET_GLOBAL: {
      int32_t global_index = (int32_t)READ_CHUNK_BYTE();
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (VALUE_IS_UNDEFINED(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      STACK_PUSH(value);
      break;
    }

    case OP_GET_GLOBAL_LONG: {
      int32_t global_index = READ_24BIT_OPERANDS();
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      STACK_PUSH(value);
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
      ant_value_array.write_at(&vm->globals, STACK_PEEK(0), global_index);
      break;
    }

    case OP_SET_GLOBAL_LONG: {
      int32_t global_index = READ_24BIT_OPERANDS();
      Value value = ant_value_array.at(&vm->globals, global_index);

      if (ant_value.is_undefined(value)) {
        runtime_error(vm, "Undefined variable");
        return INTERPRET_RUNTIME_ERROR;
      }

      ant_value_array.write_at(&vm->globals, STACK_PEEK(0), global_index);
      break;
    }

    case OP_CONSTANT:{
      STACK_PUSH(READ_CHUNK_CONSTANT());
      break;
    }

    case OP_CONSTANT_LONG: {
      STACK_PUSH(READ_CHUNK_LONG_CONSTANT());
      break;
    }
    }

   frame->ip = ip;

#undef READ_CHUNK_BYTE
#undef READ_CHUNK_CONSTANT
#undef BINARY_OP
#undef READ_24BIT_OPERANDS
#undef READ_16BIT_OPERANDS
  }

}


static bool call_value(VM *vm, Value callee, int32_t arg_count) {
   if(ant_value.is_object(callee)){
      switch(ant_object.type(callee)){

         case OBJ_FUNCTION:
            runtime_error(vm, "Functions are always type OBJ_CLOSURE.");
            return false;

         case OBJ_CLOSURE: 
            return call(vm, CLOSURE_FROM_VALUE(callee), arg_count);
            
         case OBJ_NATIVE:{
            ObjectNative *native = ant_native.from_value(callee);
            Value result         = native->func(arg_count, STACK_TOP() - arg_count);

            STACK_DECREMENT_TOP(arg_count + 1);
            STACK_PUSH(result);
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

static bool call(VM *vm, ObjectClosure *closure, int32_t arg_count) {

   if(arg_count != closure->func->arity){
      runtime_error(vm, "Expected %d arguments but got %d", closure->func->arity, arg_count);
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
   frame->closure = closure;
   frame->ip = closure->func->chunk.code;
   // position the slots to be just below the arguments, on function call
   // -1 is to account for stack slot 0 which is reserved for the VM/method calls.
   frame->slots = STACK_TOP() - arg_count - 1;
   vm->frame_count++;
   return true;
}

static void runtime_error(VM *vm, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for(int32_t i = vm->frame_count -1; i >=0; i--){

    CallFrame *frame = &vm->frames[i];

    ObjectFunction *func = frame->closure->func;
    /* -1 because interpreter is one step ahead when error occurs */
    size_t instruction   = frame->ip - frame->closure->func->chunk.code - 1;  
    int32_t line         = ant_line.get(&frame->closure->func->chunk.lines, instruction);

    fprintf(stderr, "[line %d] in ", line);
   
    if(func->name == NULL){
       fprintf(stderr, "script\n");

    } else {
       fprintf(stderr, "%s()\n", func->name->chars);
    }

  }

  STACK_RESET();
}

