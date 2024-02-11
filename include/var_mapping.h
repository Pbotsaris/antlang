#ifndef ANT_VAR_MAPPING_H
#define ANT_VAR_MAPPING_H

#include "common.h"
#include "table.h"
#include "value_array.h"

typedef struct {
   int32_t count;
   Table table;
   ValueArray reverse_lookup;
}VarMapping;


typedef struct {
   void(*init)(VarMapping*);
   void(*free)(VarMapping*);
   Value(*add)(VarMapping*, ObjectString*);
   ObjectString*(*find_name)(VarMapping*, int32_t);

}VarMappingAPI;

extern const VarMappingAPI ant_mapping;


#endif // ANT_VARIABLES_H
