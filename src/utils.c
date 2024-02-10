#include "utils.h"
#include <stdio.h>

static int32_t unpack_int32(uint8_t *bytes, int32_t num_bytes);


Utils ant_utils = {
    .unpack_int32 = unpack_int32
};


/**
 * @brief converts a byte array to a 32 bit integer
 * @param bytes the byte array to convert
 * @param num_bytes the number of bytes to convert. Must be <= 4.
 * @return the converted 32 bit integer
 */
static int32_t unpack_int32(uint8_t *bytes, int32_t num_bytes){

   if(num_bytes > 4){
       printf("Error: ant_utils.unpack_int32 can only unpack up to 4 bytes\n");
       return -1;
   }

    int32_t result = 0;

     num_bytes--;
     result = bytes[num_bytes]; // highest byte first then we shift...

     for(int i = num_bytes; i >= 0; i--){
         result = (result << 8) | bytes[i];
     }
                              

     return result;
}
