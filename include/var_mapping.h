#ifndef ANT_VAR_MAPPING_H
#define ANT_VAR_MAPPING_H

#include "common.h"
#include "table.h"
#include "strings.h"

typedef struct {
   int32_t count;
   Table table;
}VarMapping;


typedef struct {
   void(*init)(VarMapping*);
   void(*free)(VarMapping*);
   Value(*add)(VarMapping*, ObjectString*);

}VarMappingAPI;

extern const VarMappingAPI ant_mapping;


#endif // ANT_VARIABLES_H
