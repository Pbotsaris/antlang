#ifndef ANT_CHUNK_H
#define ANT_CHUNK_H
#include "common.h"
#include "lines.h"
#include "values.h"

typedef enum {
  OP_RETURN,        /* no operand */
  OP_CONSTANT,      /* 8-bit operand */
  OP_CONSTANT_LONG, /* 24-bit operand */
} OpCode;

/**
 * @brief Represents a chunk of bytecode in antlang interpreter.
 *
 * Contains bytecode instructions, constants used in the bytecode, 
 * and line number information for error reporting.
 */
typedef struct {
    int32_t capacity;     /**< The total allocated capacity for the bytecode and associated data. */
    int32_t count;        /**< The current number of bytecode instructions in the chunk. */
    ValueArray constants; /**< An array of constants used in the bytecode. */
    Lines lines;          /**< Mapping of each bytecode instruction to its line number in the source code. */
    uint8_t *code;        /**< The array of bytecode instructions. */
} Chunk;

typedef struct AntChunk {

  /**
   * @brief Initialize a chunk
   * @param chunk chunk to initialize
   */
  void (*init)(Chunk *chunk);

  /**
   * @brief Write a byte to the chunk, dynamically growing the array if needed.
   * @param chunk chunk to write to
   * @param byte byte to write
   * @param line line number of the byte
   * @details This function will automatically resize the internal array of instructions if the capacity is exceeded.
   */
  void (*write)(Chunk *chunk, uint8_t byte, int32_t line);

  /**
   * @brief free a byte to the chunk
   * @param chunk the chunk to free
   */
  void (*free)(Chunk *chunk);

  /**
   * @brief  writes a constant (OP_CONTANT or OP_CONSTANT_LONG) value to the chunk.
   * @param chunk the chunk to write to
   * @param value the constant value to write
   * @param line the line number of the constant
   */
  void (*write_constant)(Chunk *chunk, Value value, int32_t line);
} AntChunkAPI;

extern AntChunkAPI ant_chunk;

#endif
