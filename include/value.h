#ifndef ANT_VALUE_H
#define ANT_VALUE_H
#include "common.h"

/* NOTE: The reason to use an enum (and not a uint8_t) here is because we have a
 * union with a double so the compiler will add padding to the struct to align
 * the double to 8 bytes.
 * */
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_UNDEFINED,
  VAL_NUMBER,
  VAL_OBJECT,
} ValueType;

typedef struct Object Object;
typedef struct ObjectString ObjectString;
typedef struct ObjectFunction ObjectFunction;
typedef struct ObjectClosure ObjectClosure;
typedef struct ObjectNative ObjectNative;

/**
 * Represents a value in the Ant language.
 * This union allows a Value to store either a boolean, a double, or represent a
 * nil value, depending on the ValueType.
 */
typedef struct {
  ValueType type;   /**< The type of the value, determining which part of the union is used. */
  union {
    bool boolean;  /**< Storage for boolean values. Valid when type is VAL_BOOL. */
    double number; /**< Storage for numeric values. Valid when type is VAL_NUMBER. */
    Object *object; /**< Storage for dynamically allocated data. */
  } as;
} Value;

/**
 * API for creating and inspecting Ant Values.
 * This struct provides function pointers for creating Values from native C
 * types and for inspecting the type and content of existing Values.
 */
typedef struct AntValue {
  Value   (*from_bool)     (bool value);  
  Value   (*from_number)   ( double value);
  Value   (*from_object)   (Object *object);
  Value   (*make_nil)      (void);       
  Value   (*make_undefined)(void);

  double  (*as_number)     (Value value); 
  bool    (*as_bool)       (Value value); 
  Object* (*as_object)     (Value value);

  bool    (*is_bool)      (Value value);  
  bool    (*is_nil)       (Value value);   
  bool    (*is_undefined) (Value value);
  bool    (*is_number)    (Value value);    
  bool    (*is_object)    (Value value);     

  Value   (*is_falsey)    (Value value);      
  bool    (*is_falsey_bool)(Value value);      
  Value   (*equals)       (Value a, Value b);   
  int32_t (*print)        (Value value, bool debug);         
} ValueAPI;

#define VALUE_FROM_NUMBER(value)      ((Value){.type = VAL_NUMBER, .as.number = value})
#define VALUE_FROM_BOOL(value)        ((Value){.type = VAL_BOOL, .as.boolean = value})
#define VALUE_FROM_OBJECT(value)      ((Value){.type = VAL_OBJECT, .as.object = value})
#define VALUE_FROM_NIL()              ((Value){.type = VAL_NIL, .as.number = 0 })
#define VALUE_FROM_UNDEFINED()        ((Value){.type = VAL_UNDEFINED, .as.number = 0 })
#define VALUE_AS_BOOL(value)          ((value).as.boolean)
#define VALUE_AS_NUMBER(value)        ((value).as.number)
#define VALUE_AS_OBJECT(value)        ((value).as.object)
#define VALUE_IS_BOOL(value)          ((value).type == VAL_BOOL)
#define VALUE_IS_NUMBER(value)        ((value).type == VAL_NUMBER)
#define VALUE_IS_OBJECT(value)        ((value).type == VAL_OBJECT)
#define VALUE_IS_NIL(value)           ((value).type == VAL_NIL)
#define VALUE_IS_UNDEFINED(value)     ((value).type == VAL_UNDEFINED)
#define VALUE_IS_FALSEY(value)        (VALUE_FROM_BOOL(VALUE_IS_NIL(value) || (VALUE_IS_BOOL(value) && !(VALUE_AS_BOOL(value)))))
#define VALUE_IS_FALSEY_AS_BOOL(value)(VALUE_IS_NIL(value) || (VALUE_IS_BOOL(value) && !(VALUE_AS_BOOL(value))))

#define VALUE_EQUALS(a, b) \
  ((a).type != (b).type ? VALUE_FROM_BOOL(false) : \
   (a).type == VAL_NIL ? VALUE_FROM_BOOL(true) : \
   (a).type == VAL_BOOL ? VALUE_FROM_BOOL((a).as.boolean == (b).as.boolean) : \
   (a).type == VAL_NUMBER ? VALUE_FROM_BOOL((a).as.number == (b).as.number) : \
   (a).type == VAL_OBJECT ? VALUE_FROM_BOOL((a).as.object == (b).as.object) : \
   VALUE_FROM_BOOL(false))

extern ValueAPI ant_value;

#endif //ANT_VALUE_H
