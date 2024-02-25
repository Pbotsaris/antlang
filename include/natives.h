#ifndef ANT_NATIVES_H
#define ANT_NATIVES_H

#include "object.h"
#include "value.h"
#include "vm.h"

typedef Value (*NativeFunction)(int32_t arg_count, Value *args);

struct ObjectNative {
  Object object;
  NativeFunction func;
};

typedef struct {
   Object*        (*as_object)(ObjectNative *native);
   ObjectNative*  (*new)(NativeFunction func);
   ObjectNative*  (*from_value)(Value value);
   void           (*register_all)(VM *vm);
   int32_t        (*print)(void);
}ObjectNativeAPI;

#define NATIVE_AS_OBJECT(native) ((Object*)(native))
#define NATIVE_FROM_VALUE(value) ((ObjectNative*)VALUE_AS_OBJECT(value))

const extern ObjectNativeAPI ant_native;

#endif // ANT_NATIVES_H
