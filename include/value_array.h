#ifndef ANT_VALUES_H
#define ANT_VALUES_H

#include "common.h"
#include "value.h"

/**
 * Represents an array of constant values used in bytecode.
 */
typedef struct {
  int32_t capacity;  /**< Total allocated capacity for storing constant values. */
  int32_t count;    /**< Number of constant values currently stored in the array. */
  Value *values;    /**< Pointer to the array of constant values. */
  bool has_nils;
} ValueArray;

/**
 * @brief API for managing an array of constant values.
*/
typedef struct AntValuesArray {
  /** @brief Initializes a ValueArray structure.
   * @param array Pointer to the ValueArray structure to initialize.
   */
  void (*init)(ValueArray *array);;

  /**
   * @brief Initializes a ValueArray structure with nils. When array grows to new capacity, it will be filled with nils.
   * @param array Pointer to the ValueArray structure to initialize.
   */
  void (*init_nils)(ValueArray *array);

  /**
   * @brief Writes a Value to the array.
   * @param array Pointer to the ValueArray structure.
   * @param value The constant value to be added to the array.
   * @details This function adds a Value to the array and adjusts its
   * size if necessary.
   */
  void (*write)(ValueArray *array, Value value);

  /**
   * @brief Writes a constant value to the array at a specific index. Should be using always in conjuction with write_at and initating the array with init_nils.
   * @param array Pointer to the ValueArray structure.
   * @param value The constant value to be added to the array.
   * @param index The index at which to add the constant value.
   * @details This function adds a Value to the array at a specific index
   * and adjusts its size if necessary.
   */
  bool (*write_at)(ValueArray *array,  Value value, int32_t index);


  /**
   * @brief Reads a constant value from the array at a specific index. Should be using always in conjuction with write_at and initating the array with init_nils.
   * @param array Pointer to the ValueArray structure.
   * @param index The index from which to read the constant value.
   * @return The constant value at the specified index. Returns nil if the index is out of bounds.
   */
  Value (*at)(ValueArray *array, int32_t index);

  /**
   * @brief Frees resources in a ValueArray structure.
   * @param array Pointer to the ValueArray structure to free.
   */
  void (*free)(ValueArray *array);


} ValuesArrayAPI;

extern ValuesArrayAPI ant_value_array;

#endif // ANT_VALUES_H
