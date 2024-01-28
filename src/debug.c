#include "debug.h"
#include "lines.h"
#include "values.h"
#include "utils.h"

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

  int32_t chunk_line = ant_line.get(&chunk->lines, offset);

  bool same_line =
      offset > 0 && chunk_line == ant_line.get(&chunk->lines, offset - 1);

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

  case OP_CONSTANT_LONG:
    return print_constant_instruction("OP_CONSTANT_LONG", chunk, offset);

  default:
    printf("Unknown opcode '%d'\n", instruction);
    return offset + 1;
  }
}

static int print_instruction(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int print_constant_instruction(const char *name, Chunk *chunk, int offset) {

  int32_t const_index    = 0;
  int32_t operand_offset = 0;

  /* putting 24 bits together to get the constant index */
  if (chunk->code[offset] == OP_CONSTANT_LONG) {
    uint8_t *operand_bytes = chunk->code + offset + 1;
    const_index = ant_utils.unpack_int32(operand_bytes, 3);
    operand_offset = 4; // 1 opcode + 3 operands

  } else {
    const_index = (int32_t)chunk->code[offset + 1];
    operand_offset = 2; // 1 opcode + 1 operand
  }

  /* index:value */
  printf("%-16s %4d:", name, const_index);

  if(const_index >= chunk->constants.count) {
    printf("Unknown constant index '%d'\n", const_index);
    return offset + operand_offset;
  }

  print_value(chunk->constants.values[const_index]);

  return offset + operand_offset;
}
