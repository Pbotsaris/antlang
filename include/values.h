#ifndef ANT_VALUES_H
#define ANT_VALUES_H

#include "common.h"

typedef double Value;

/**
 * @brief Represents an array of constant values used in bytecode.
 */
typedef struct {
    int32_t capacity; /**< Total allocated capacity for storing constant values. */
    int32_t count;    /**< Number of constant values currently stored in the array. */
    Value *values;    /**< Pointer to the array of constant values. */
} ValueArray;

/**
 * @brief API for managing an array of constant values.
 */
typedef struct AntValues {
   /**
    * @brief Initializes a ValueArray structure.
    * @param array Pointer to the ValueArray structure to initialize.
    */
   void (*init)(ValueArray *array);

   /**
    * @brief Writes a constant value to the array.
    * @param array Pointer to the ValueArray structure.
    * @param value The constant value to be added to the array.
    * @details This function adds a constant value to the array and adjusts its size if necessary.
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

extern ValuesArrayAPI ant_values;

#endif // ANT_VALUES_H
