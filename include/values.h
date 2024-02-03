#ifndef ANT_VALUES_H
#define ANT_VALUES_H

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
 * Represents an array of constant values used in bytecode.
 */
typedef struct {
  int32_t capacity;  /**< Total allocated capacity for storing constant values. */
  int32_t count;    /**< Number of constant values currently stored in the array. */
  Value *values;    /**< Pointer to the array of constant values. */
} ValueArray;

/**
 * @brief API for managing an array of constant values.
*/
typedef struct AntValuesArray {
  /** @brief Initializes a ValueArray structure.
   * @param array Pointer to the ValueArray structure to initialize.
   */
  void (*init)(ValueArray *array);

  /**
   * @brief Writes a constant value to the array.
   * @param array Pointer to the ValueArray structure.
   * @param value The constant value to be added to the array.
   * @details This function adds a constant value to the array and adjusts its
   * size if necessary.
   */
  void (*write)(ValueArray *array, Value value);

  /**
   * @brief Frees resources in a ValueArray structure.
   * @param array Pointer to the ValueArray structure to free.
   */
  void (*free)(ValueArray *array);

  /**
   * @brief Prints a constant value to the standard output.
   * @param value The constant value to be printed.
   */
  void (*print)(Value value);
} ValuesArrayAPI;

/**
 * API for creating and inspecting Ant Values.
 * This struct provides function pointers for creating Values from native C
 * types and for inspecting the type and content of existing Values.
 */
typedef struct AntValue {
  Value  (*from_bool)(bool value);      /**< Creates a Value from a C boolean. */
  Value  (*from_number)( double value); /**< Creates a Value from a numeric (C double) value. */
  Value  (*from_nil)(void);             /**< Creates a Value representing nil. */
  bool   (*to_bool)(Value value);       /**< Converts a Value to a C boolean. Valid if is_bool(value) is true. */
  double (*to_number)(Value value);     /**< Converts a Value to a C number. Valid if is_number(value) is true. */
  bool   (*is_bool)(Value value);       /**< Checks if a Value is of boolean type. */
  bool   (*is_nil)(Value value);        /**< Checks if a Value is of nil type. */
  bool   (*is_number)(Value value);     /**< Checks if a Value is of numeric type. */
} ValueAPI;

extern ValuesArrayAPI ant_values;
extern ValueAPI ant_value;

#endif // ANT_VALUES_H
