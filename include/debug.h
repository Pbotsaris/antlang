#ifndef ANT_DEBUG_H
#define ANT_DEBUG_H

#include "token.h"
#include "compiler.h"

typedef struct AntDebug {
  /**
   * @brief Prints bytecode instructions, line numbers, and constants value (if any) for a chunk.
   *
   * @param chunk A pointer to the Chunk to disassemble.
   * @param name  A string identifier for the chunk.
   */
  void (*disassemble_chunk)(Compiler *compiler, Chunk *frame_chunk, const char *name);


  int (*disassemble_instruction)(Compiler *compiler, Chunk *frame_chunk, int32_t offset);
  void (*trace_parsing)(const char *func_name, int32_t depth, const char *format, ...);
  void (*trace_tokens)(Token prev, Token current, int32_t depth);
} DebugAPI;

extern DebugAPI ant_debug;

#endif
