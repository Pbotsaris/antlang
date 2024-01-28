#ifndef ANT_UTILS_H
#define ANT_UTILS_H
#include "common.h"

/**
 * @brief Utility functions for the  Antlang interpreter
 */
typedef struct AntUtils {
  /**
   * @brief converts a byte array to a 32 bit integer
   * @param bytes the byte array to convert
   * @param num_bytes the number of bytes to convert. Must be <= 4.
   * @return the converted 32 bit integer
   */
  int32_t (*unpack_int32)(uint8_t *, int32_t);
} Utils;

extern Utils ant_utils;

#endif
