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
} ValueType;

/**
 * Represents a value in the Ant scripting language.
 * This union allows a Value to store either a boolean, a double, or represent a
 * nil value, depending on the ValueType.
 */
typedef struct {
  ValueType type;   /**< The type of the value, determining which part of the union is used. */
  union {
    bool boolean;  /**< Storage for boolean values. Valid when type is VAL_BOOL. */
    double number; /**< Storage for numeric values. Valid when type is VAL_NUMBER. */
  } as;
} Value;

/**
 * API for creating and inspecting Ant Values.
 * This struct provides function pointers for creating Values from native C
 * types and for inspecting the type and content of existing Values.
 */
typedef struct AntValue {
  Value  (*from_c_bool)(bool value);      /**< Creates a Value from a C boolean. */
  Value  (*from_c_number)( double value); /**< Creates a Value from a numeric (C double) value. */
  Value  (*as_nil)(void);             /**< Creates a Value representing nil. */
  Value  (*is_falsey)(Value value);       /**< Checks if a Value is falsey (nil or false). */
  Value  (*equals)(Value a, Value b);     /**< Checks if two Values are equal. */
  double (*to_c_number)(Value value);     /**< Converts a Value to a C number. Valid if is_number(value) is true. */
  void   (*print)(Value value);           /**< Prints a Value to the console. */
  bool   (*to_c_bool)(Value value);       /**< Converts a Value to a C boolean. Valid if is_bool(value) is true. */
  bool   (*is_bool)(Value value);         /**< Checks if a Value is of boolean type. */
  bool   (*is_nil)(Value value);          /**< Checks if a Value is of nil type. */
  bool   (*is_number)(Value value);       /**< Checks if a Value is of numeric type. */
} ValueAPI;

extern ValueAPI ant_value;
#endif //ANT_VALUE_H
