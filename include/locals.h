#ifndef ANT_LOCAL_VARIABLES_H
#define ANT_LOCAL_VARIABLES_H

#include "common.h"
#include "config.h"
#include "token.h"
#include "chunk.h"


#define LOCALS_UNINITIALIZED -1
#define LOCALS_NOT_FOUND -2

typedef void(*ClearCallback)(OpCode);

typedef struct {
   Token name;
   int32_t depth; // how deep in the stack the variable is
}Local;

typedef struct {
 Local locals[OPTION_STACK_MAX];
 int32_t count;
 int32_t depth;
}LocalStack;


typedef struct {
   void        (*init)             (LocalStack* stack);
   void        (*push)             (LocalStack* stack, Token name);
   bool        (*validate_scope)   (LocalStack* stack, Token *name);
   void        (*mark_initialized) (LocalStack* stack);
   void        (*clear_scope)      (LocalStack* stack, ClearCallback callback);
   int32_t     (*resolve)          (LocalStack* stack, Token *name);
   void        (*print)            (LocalStack* stack, int32_t index);
}LocalStackAPI;

const extern LocalStackAPI ant_locals;
#endif
