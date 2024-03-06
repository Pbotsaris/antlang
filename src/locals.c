#include "locals.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Public */


static void init_local_stack(LocalStack *stack);
static void push_local_stack(LocalStack *stack, Token name);
static Local get_top(LocalStack *stack);
static ScopeType current_scope(LocalStack *stack);
static bool validate_scope(LocalStack *stack, Token *name);
static void mark_initialized(LocalStack *stack);
static void mark_captured(LocalStack *stack, int32_t local_index);
static int32_t resolve_local(LocalStack *stack, Token *name);
static int32_t print_local_name(LocalStack *stack, int32_t local_index);

const LocalStackAPI ant_locals = {
   .init = init_local_stack,
   .push = push_local_stack,
   .current_scope = current_scope,
   .validate_scope = validate_scope,
   .mark_initialized = mark_initialized,
   .mark_captured = mark_captured,
   .resolve = resolve_local,
   .print = print_local_name,
   .top = get_top,
};

/* Private */
static bool token_compare(Token *a, Token *b);

/* Implementation */
static void init_local_stack(LocalStack *stack) {
   // Claim the stack slot 0 for the VM internal use
   Local *local = &stack->locals[0];
   local->name.start  = "";
   local->name.length = 0;
   local->is_captured = false;

   /* this slot of for function and be part of the compiler:end_scope logc 
    * so we set the depth to -1 so functions or script don't get pop in end_scope */
   local->depth = -1; 

   stack->count = 1;
   stack->depth = 0;
}

static void push_local_stack(LocalStack *stack, Token name){
   Local *local       = &stack->locals[stack->count];
   local->name        = name;
   local->is_captured = false;
   local->depth       = LOCALS_NOT_INTIALIZED;  // we must manually mark the local as initialized
   stack->count++;
}

static ScopeType current_scope(LocalStack *stack){

   if(stack->depth == 0){
      return SCOPE_GLOBAL;
   }

   if(stack->depth > 0){
      return SCOPE_LOCAL;
   }

   return SCOPE_INVALID;
}

static bool validate_scope(LocalStack *stack, Token *name){
   for(int32_t i = stack->count - 1; i >= 0; i--){

      Local *local = &stack->locals[i];

      // loops backwards, break we find a local that is not initialized or is in a lower scope
      if(local->depth != LOCALS_NOT_INTIALIZED && local->depth < stack->depth){
         break;
      }

      if(token_compare(&local->name, name)){
         return false;
      }
   }

   return true;
}


static int32_t resolve_local(LocalStack *stack, Token *name){
   /* start from the top of the stack and work our way down */
   /* most inner scope is at the top of the stack */
   for(int32_t i = stack->count - 1; i >= 0; i--){
      Local *local  = &stack->locals[i];
     
      if(token_compare(&local->name, name)){

         if(local->depth == LOCALS_NOT_INTIALIZED){
            return LOCALS_NOT_INTIALIZED;
         }

         return  i;
      }
   }

   return LOCALS_NOT_FOUND;
}

static void mark_initialized(LocalStack *stack){
   stack->locals[stack->count - 1].depth = stack->depth - 1;
}

static int32_t print_local_name(LocalStack *stack, int32_t index){

   if(index == LOCALS_NOT_FOUND || index == LOCALS_NOT_INTIALIZED || index > stack->count - 1){
      return printf("Local{ N/A }");
   }

   const char *name = stack->locals[index].name.start;

   //TODO: How to handle local function variables?
   if(name == NULL){
      return printf("Local{ N/A }");
   }

   return printf("Local{ '%.*s' }", stack->locals[index].name.length, name);
}

static Local get_top(LocalStack *stack){
   return stack->locals[stack->count-1];
}

static void mark_captured(LocalStack *stack, int32_t local_index){
   assert(local_index < stack->count);
   stack->locals[local_index].is_captured = true;
}


static bool token_compare(Token *a, Token *b){
   if(a->length != b->length){
      return false;
   }

   return memcmp(a->start, b->start, a->length) == 0;
}
