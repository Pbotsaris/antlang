#ifndef ANT_LINES_H
#define ANT_LINES_H

#include "common.h"

/**
 * @brief Represents a mapping between a range of bytecode instructions and a line number in the source code.
 */
typedef struct {
    int32_t start;  /**< The starting index in the bytecode array where this line range begins. */
    int32_t end;    /**< The ending index in the bytecode array where this line range ends. */
    int32_t number; /**< The line number in the source code corresponding to this range of bytecode. */
} Line;

/**
 * @brief Represents a collection of line mappings for a chunk of bytecode.
 *
 * This structure holds an array of `Line` structures, each representing a range of bytecode
 * instructions and their corresponding line number in the source code. It's used to associate
 * each instruction in a chunk of bytecode with a specific line in the source code.
 */
typedef struct {
    int32_t capacity; /**< The total allocated capacity for the lines array. */
    int32_t count;    /**< The current number of line mappings in the lines array. */
    Line *lines;      /**< The array of `Line` structures representing line mappings. */
} Lines;


/**
 * @brief API for managing line number mappings in bytecode.
 */
typedef struct AntLine {
   /**
    * @brief Initializes a Lines structure.
    * @param lines Pointer to the Lines structure to initialize.
    */
   void (*init)(Lines *lines);
   /**
    * @brief Writes a line number association for a given chunk index, dynamically growing the array if needed.
    * @param lines Pointer to the Lines structure.
    * @param line Line number in the source code.
    * @param chunk_index Index in the bytecode array.
    * @details This function will automatically resize the internal array of lines if the capacity is exceeded.
    */
   void (*write)(Lines *lines, int32_t line, int32_t chunk_index);

   /**
    * @brief Retrieves the source code line number for a bytecode index.
    * @param lines Pointer to the Lines structure.
    * @param chunk_index Index in the bytecode array.
    * @return Line number in the source code.
    */
   int32_t (*get)(Lines *lines, int32_t chunk_index);

   /**
    * @brief Frees resources in a Lines structure.
    * @param lines Pointer to the Lines structure to free.
    */
   void (*free)(Lines *lines);
} AntLineAPI;

extern AntLineAPI ant_line;

#endif // ANT_LINES_H
