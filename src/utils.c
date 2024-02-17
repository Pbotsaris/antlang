#include "utils.h"
#include <stdio.h>

static int32_t unpack_int32(uint8_t *bytes, int32_t num_bytes);
static uint16_t unpack_uint16(uint8_t *bytes);

Utils ant_utils = {
    .unpack_int32 = unpack_int32,
    .unpack_uint16 = unpack_uint16,
};

/**
 * @brief converts a byte array to a 32 bit integer
 * @param bytes the byte array to convert
 * @param num_bytes the number of bytes to convert. Must be <= 4.
 * @return the converted 32 bit integer
 */
static int32_t unpack_int32(uint8_t *bytes, int32_t num_bytes) {

  if (num_bytes > 4) {
    printf("Error: ant_utils.unpack_int32 can only unpack up to 4 bytes\n");
    return -1;
  }

  if (num_bytes == 0) {
    return 0;
  }

  int32_t shift_pos = num_bytes - 1;
  int32_t shifts[] = {0, 8, 16, 24};
  int32_t result = bytes[0] << shifts[shift_pos];

  for (int i = 1; i < num_bytes; i++) {
    result |= bytes[i] << shifts[--shift_pos];
  }

  return result;
}

static uint16_t unpack_uint16(uint8_t *bytes) {
  return (uint16_t)(bytes[0] << 8 | bytes[1]);
}

// let i = 0; while( i < 4) { print i; i = i + 1;}
