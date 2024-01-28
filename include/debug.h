#ifndef ANT_DEBUG_H
#define ANT_DEBUG_H

#include "chunk.h"

typedef struct AntDebug {
  /**
   * @brief Prints bytecode instructions, line numbers, and constants value (if any) for a chunk.
   *
   * @param chunk A pointer to the Chunk to disassemble.
   * @param name  A string identifier for the chunk.
   */
  void (*disassemble_chunk)(Chunk *chunk, const char *name);
  int (*disassemble_instruction)(Chunk *chunk, int32_t offset);
} DebugAPI;

extern DebugAPI ant_debug;

#endif
