#include "natives.h"
#include "var_mapping.h"
#include "value_array.h"
#include "stack.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

static ObjectNative*   new_native(NativeFunction func);
static int32_t         print_native(void);
static ObjectNative*   native_from_value(Value value);
static Object*         native_as_object(ObjectNative* native);
static void            register_all_natives(VM *vm);

const ObjectNativeAPI ant_native = {
    .new          = new_native,
    .print        = print_native,
    .from_value   = native_from_value,
    .as_object    = native_as_object,
    .register_all = register_all_natives,
};

/* Private */
static void define_native_function(VM *vm, const char *name, NativeFunction func);

/* Native Functions */
static Value native_clock(int32_t arg_count, Value *args);

/* API Implementation */

static ObjectNative *new_native(NativeFunction func){
   ObjectNative *native = (ObjectNative*)ant_object.allocate(sizeof(ObjectNative), OBJ_NATIVE);
   native->func = func;

   return native;
}

static ObjectNative* native_from_value(Value value){
  return (ObjectNative*)ant_value.as_object(value);
}


static Object* native_as_object(ObjectNative* native){
  return (Object*)native;
}

static int32_t print_native(void){
   return printf("<native fn>");
}

static void register_all_natives(VM *vm){
   define_native_function(vm, "clock", native_clock);
}

/* Private */

static void define_native_function(VM *vm, const char *name, NativeFunction func) {
   ObjectString *func_name    = ant_string.new(name, (int32_t)strlen(name));
   ObjectNative *native_func  = ant_native.new(func);

   STACK_PUSH(ant_value.from_object(ant_string.as_object(func_name)));
   STACK_PUSH(ant_value.from_object(ant_native.as_object(native_func)));

   int32_t global_index = ant_value.as_number(ant_mapping.add(func_name));
   ant_value_array.write_at(&vm->globals, STACK_AT(1), global_index);
   
   STACK_POP();
   STACK_POP();
}

/* Native Functions */

static Value native_clock(int32_t arg_count, Value *args){
   return ant_value.from_number((double)clock()/CLOCKS_PER_SEC);
}
