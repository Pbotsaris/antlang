#include "debug.h"
#include "lines.h"
#include "values.h"

#include <stdio.h>

static int print_instruction(const char *name, int offset);
static int disassemble_instruction(Chunk *chunk, int offset);
static int print_constant_instruction(const char *name, Chunk *chunk,
                                      int offset);

void disassemble_chunk(Chunk *chunk, const char *name) {
  printf("-- %s --\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassemble_instruction(chunk, offset);
  }
}

static int disassemble_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  int32_t chunk_line = get_line(&chunk->lines, offset);

  bool same_line =
      offset > 0 && chunk_line == get_line(&chunk->lines, offset - 1);
  

  if (same_line)
    printf("   | ");
  else
    printf("%4d ", chunk_line);

  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
  case OP_RETURN:
    return print_instruction("OP_RETURN", offset);

  case OP_CONSTANT:
    return print_constant_instruction("OP_CONSTANT", chunk, offset);

  default:
    printf("Unknown opcode '%d'\n", instruction);
    return offset + 1;
  }
}

static int print_instruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int print_constant_instruction(const char *name, Chunk *chunk,
                                      int offset) {

  /* print constant index (in values array) and the value */
  uint8_t const_index = chunk->code[offset + 1];
  printf("%-16s %4d:", name, const_index);
  print_value(chunk->constants.values[const_index]);

  return offset + 2;
}
