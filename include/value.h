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
  VAL_NUMBER,
  VAL_OBJECT,
} ValueType;

typedef struct Object Object;
typedef struct ObjectString ObjectString;

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
  Value  (*make_bool)     (bool value);  
  Value  (*make_number)   ( double value);
  Value  (*make_object)   (Object *object);
  Value  (*make_nil)      (void);       

  double (*as_number)     (Value value); 
  bool   (*as_bool)       (Value value); 
  Object*(*as_object)     (Value value);

  bool   (*is_bool)      (Value value);  
  bool   (*is_nil)       (Value value);   
  bool   (*is_number)    (Value value);    
  bool   (*is_object)    (Value value);     

  Value  (*is_falsey)    (Value value);      
  Value  (*equals)      (Value a, Value b);   
  void   (*print)       (Value value);         
} ValueAPI;

extern ValueAPI ant_value;

#endif //ANT_VALUE_H
