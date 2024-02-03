#include "debug.h"
#include "config.h"
#include "lines.h"
#include "utils.h"
#include "values.h"

#include <stdarg.h>
#include <stdio.h>

static void disassemble_chunk(Chunk *chunk, const char *name);
static int disassemble_instruction(Chunk *chunk, int offset);
static void trace_parsing(const char *func_name, int32_t depth,
                          const char *format, ...);
static void trace_tokens(Token prev, Token current, int32_t depth);
static void trace_token(Token token, const char *name, int32_t depth);

DebugAPI ant_debug = {
    .disassemble_chunk = disassemble_chunk,
    .disassemble_instruction = disassemble_instruction,
    .trace_parsing = trace_parsing,
    .trace_tokens = trace_tokens,
};

/* helpers */
static int32_t print_instruction(const char *name, int32_t offset);
static int32_t print_constant_instruction(const char *name, Chunk *chunk,
                                          int32_t offset);

static void disassemble_chunk(Chunk *chunk, const char *name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassemble_instruction(chunk, offset);
  }

  printf("\n");
}

static void trace_parsing(const char *func_name, int32_t depth,
                          const char *format, ...) {

  va_list args;
  va_start(args, format);

  for (int32_t i = 0; i < depth; i++) {
    printf("  ");
  }

  printf("%s(", func_name);
  vprintf(format, args);
  printf(")\n");

  va_end(args);
}

static void trace_tokens(Token prev, Token current, int32_t depth) {
  trace_token(prev, "prev", depth);
  trace_token(current, "current", depth);
  printf("\n");
}

static void trace_token(Token token, const char *name, int32_t depth) {

  for (int32_t i = 0; i < depth; i++) {
    printf("  ");
  }

  if (token.start == NULL) {
     printf("%s%-*s[ NULL ]%s\n", COLOR_CYAN, 10, name, COLOR_RESET);
     return;
  }

  printf("%s%-*s", COLOR_CYAN, 10, name);
  ant_token.print(token);
  printf("%s", COLOR_RESET);
}

/* Helpers */
static int32_t disassemble_instruction(Chunk *chunk, int offset) {
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

  case OP_NEGATE:
    return print_instruction("OP_NEGATE", offset);

  case OP_POSITIVE:
    return print_instruction("OP_POSITIVE", offset);

  case OP_ADD:
    return print_instruction("OP_ADD", offset);

  case OP_SUBTRACT:
    return print_instruction("OP_SUBTRACT", offset);

  case OP_MULTIPLY:
    return print_instruction("OP_MULTIPLY", offset);

  case OP_DIVIDE:
    return print_instruction("OP_DIVIDE", offset);

  case OP_CONSTANT:
    return print_constant_instruction("OP_CONSTANT", chunk, offset);

  case OP_CONSTANT_LONG:
    return print_constant_instruction("OP_CONSTANT_LONG", chunk, offset);

  default:
    printf("Unknown opcode '%d'\n", instruction);
    return offset + 1;
  }
}

static int32_t print_instruction(const char *name, int offset) {
  printf("%-*s ", 10, name);
  return offset + 1;
}

static int print_constant_instruction(const char *name, Chunk *chunk,
                                      int offset) {
  int32_t const_index = 0;
  int32_t operand_offset = 0;

  /* putting 24 bits together to get the constant index */
  if (chunk->code[offset] == OP_CONSTANT_LONG) {
    uint8_t *operand_bytes = chunk->code + offset + 1;
    const_index = ant_utils.unpack_int32(operand_bytes, CONST_24BITS);
    operand_offset = 1 + CONST_24BITS; // 1 opcode + 3 operands

  } else {
    const_index = (int32_t)chunk->code[offset + 1];
    operand_offset = 1 + CONST_8BITS; // 1 opcode + 1 operand
  }

  /* index:value */
  printf("%-16s %4d:", name, const_index);

  if (const_index >= chunk->constants.count) {
    printf("Unknown constant index '%d'\n", const_index);
    return offset + operand_offset;
  }

  ant_values.print(chunk->constants.values[const_index]);

  return offset + operand_offset;
}
