#ifndef ANT_NATIVES_H
#define ANT_NATIVES_H

#include "object.h"
#include "value.h"
#include "vm.h"

typedef Value (*NativeFunction)(int32_t arg_count, Value *args);

typedef struct {
  Object object;
  NativeFunction func;
} ObjectNative;

typedef struct {
   Object*        (*as_object)(ObjectNative *native);
   ObjectNative*  (*new)(NativeFunction func);
   ObjectNative*  (*from_value)(Value value);
   void           (*register_all)(VM *vm);
   int32_t        (*print)(void);
}ObjectNativeAPI;

const extern ObjectNativeAPI ant_native;

#endif // ANT_NATIVES_H
