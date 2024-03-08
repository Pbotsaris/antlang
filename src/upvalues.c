#include "upvalues.h"

static ObjectUpvalue* new_upvalue(Value *stack_slot);
static ObjectUpvalue* capture_upvalue(UpvalueList *open_upvalues, Value *stack_slot);
static void           close_upvalue(UpvalueList *open_upvalues, Value *stack_top); 

const UpvalueAPI ant_upvalues = {
   .new = new_upvalue,
   .capture = capture_upvalue,
   .close = close_upvalue
};

static ObjectUpvalue* new_upvalue(Value *stack_slot){
   ObjectUpvalue *upvalue = (ObjectUpvalue*)ant_object.allocate(sizeof(ObjectUpvalue), OBJ_UPVALUE);
   upvalue->location      = stack_slot;
   upvalue->next          = NULL;
   upvalue->closed        = ant_value.make_nil();

   return upvalue;
}

static ObjectUpvalue *capture_upvalue(UpvalueList *open_upvalues, Value *stack_slot){

   /* adds higher memory addresses to the top of the list */
   ObjectUpvalue *prev     = NULL;
   ObjectUpvalue * current = open_upvalues->head;

   /* iterate to the position where the upvalue should be inserted */
   while(current != NULL && current->location > stack_slot) {
      prev    = current;
      current = current->next;
   }

   if(current != NULL && current->location == stack_slot){
      return current;
   }

   ObjectUpvalue *created_upvalue = new_upvalue(stack_slot);
   created_upvalue->next          = current;


   if(prev == NULL){
       /*  then created_upvalue is the first element and should go in the head */
      open_upvalues->head = created_upvalue;
   } else {
      prev->next = created_upvalue;
   }

   return created_upvalue;
}

static void close_upvalue(UpvalueList *open_values, Value *stack_slot){

   /* The list is sorted with higher memory addresses (stack top) near the head. 
    * So we close all the upvalues that are above the stack slot.
    **/

   while(open_values->head != NULL && open_values->head->location >= stack_slot){

      ObjectUpvalue *current = open_values->head;

      /* Preserve the stack slot's value that's being removed from the stack 
       * by storing it in the Upvalue object. Then, update location to point 
       * to this preserved value. The Upvalue now is self-contained 
       * so we removed from the open_value list.
       **/

      current->closed            = *current->location;
      current->location          = &current->closed;
      open_values->head          = current->next;
   }
}
