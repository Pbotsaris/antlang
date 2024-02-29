#ifndef ANT_CHUNK_H
#define ANT_CHUNK_H
#include "common.h"
#include "lines.h"
#include "value_array.h"

typedef enum {
  OP_RETURN,             /* no operand */
  OP_NEGATE,             /* no operand */
  OP_POSITIVE,           /* no operand */
  OP_ADD,                /* no operand */
  OP_SUBTRACT,           /* no operand */
  OP_MULTIPLY,           /* no operand */
  OP_DIVIDE,             /* no operand */
  OP_NIL,                /* no operand */
  OP_TRUE,               /* no operand */
  OP_FALSE,              /* no operand */
  OP_NOT,                /* no operand */
  OP_EQUAL,              /* no operand */
  OP_GREATER,            /* no operand */
  OP_LESS,               /* no operand */
  OP_PRINT,              /* no operand */
  OP_POP,                /* no operand */

  OP_CLOSURE,            /*  8-bit operand  + a pair of bytes per upvalue in func->upvalue_count */
  OP_CLOSURE_LONG,       /*  24-bit operand + a pair of bytes per upvalue in func->upvalue_count */

  OP_CALL,               /* 8-bit operand  */
  OP_JUMP,               /* 16-bit operand */
  OP_JUMP_IF_FALSE,      /* 16-bit operand */
  OP_LOOP,               /* 16-bit operand */

  OP_SET_UPVALUE,        /*  no operand */
  OP_GET_UPVALUE,        /*  no operand */

  OP_DEFINE_GLOBAL,      /* 8-bit operand  */
  OP_DEFINE_GLOBAL_LONG, /* 24-bit operand */
  OP_GET_GLOBAL,         /* 8-bit operand  */
  OP_GET_GLOBAL_LONG,    /* 24-bit operand */
  OP_SET_GLOBAL,         /* 8-bit operand  */
  OP_SET_GLOBAL_LONG,    /* 24-bit operand */

  OP_SET_LOCAL,          /* 8-bit operand  */
  OP_SET_LOCAL_LONG,     /* 24-bit operand */
  OP_GET_LOCAL,          /* 8-bit operand  */
  OP_GET_LOCAL_LONG,     /* 24-bit operand */

  OP_CONSTANT,           /* 8-bit operand  */
  OP_CONSTANT_LONG,      /* 24-bit operand */
} OpCode;

/**
 * @brief Represents a chunk of bytecode in antlang interpreter.
 *
 * Contains bytecode instructions, constants used in the bytecode, 
 * and line number information for error reporting.
 */
typedef struct {
    int32_t    capacity;                  /**< The total allocated capacity for the bytecode and associated data. */
    int32_t    count;                     /**< The current number of bytecode instructions in the chunk. */
    ValueArray constants;                 /**< An array of constants used in the bytecode. */
    Lines      lines;                     /**< Mapping of each bytecode instruction to its line number in the source code. */
    uint8_t*   code;                      /**< The array of bytecode instructions. */
} Chunk;

typedef struct AntChunk {

  /**
   * @brief Initialize a chunk
   * @param chunk chunk to initialize
   */
  void (*init)(Chunk *chunk);

  /**
   * @brief Write a byte to the chunk, dynamically growing the array if needed.
   * @param byte byte to write
   * @param chunk chunk to write to
   * @param line line number of the byte
   * @details This function will automatically resize the internal array of instructions if the capacity is exceeded.
   */
  void (*write)(Chunk *chunk, uint8_t byte, int32_t line);
  bool (*patch_16bits)(Chunk *chunk, int32_t offset, int32_t value);

  /**
   * @brief free a byte to the chunk
   * @param chunk the chunk to free
   */
  void (*free)(Chunk *chunk);

  int32_t (*add_constant)(Chunk *chunk, Value value);

  /**
   * @brief  writes a constant (OP_CONSTANT or OP_CONSTANT_LONG) value to the chunk
   * @param chunk the chunk to write to
   * @param value the constant value to write
   * @param line the line number of the constant
   * @returns the index of the constant in the chunk's constant array. If < 0, an error occurred.
   */
  bool (*write_constant)      (Chunk *chunk, Value value, int32_t line);
  bool (*write_closure)       (Chunk *chunk, Value value, int32_t line);
  bool (*write_define_global) (Chunk *chunk, int32_t global_index, int32_t line);
  bool (*write_get_global)    (Chunk *chunk, int32_t global_index, int32_t line);
  bool (*write_set_global)    (Chunk *chunk, int32_t global_index, int32_t line);
  bool (*write_get_local)     (Chunk *chunk, int32_t local_index, int32_t line);
  bool (*write_set_local)     (Chunk *chunk, int32_t local_index, int32_t line);
  bool (*write_get_upvalue)   (Chunk *chunk, int32_t upvalue_index, int32_t line);
  bool (*write_set_upvalue)   (Chunk *chunk, int32_t upvalue_index, int32_t line);
} AntChunkAPI;

extern AntChunkAPI ant_chunk;

#endif
