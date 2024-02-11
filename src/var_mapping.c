#include "var_mapping.h"
#include "table.h"
#include "value.h"
#include <stdio.h>

void init_mapping(VarMapping *mapping);
void free_mapping(VarMapping *mapping);
Value add_mapping(VarMapping *mapping, ObjectString *name);

const VarMappingAPI ant_mapping = {
    .init = init_mapping,
    .free = free_mapping,
    .add = add_mapping,
};

void init_mapping(VarMapping *mapping) {
  mapping->count = 0;
  ant_table.init(&mapping->table);
}

void free_mapping(VarMapping *mapping) {
  ant_table.free(&mapping->table);
  mapping->count = 0;
}

Value add_mapping(VarMapping *mapping, ObjectString *name) {
  Value value;

  bool found = ant_table.get(&mapping->table, name, &value);

     if(found) {
        return value;
     }

  value = ant_value.from_number(mapping->count);
  bool is_new = ant_table.set(&mapping->table, name, value);

  if(!is_new){
     fprintf(stderr, "Warning: Variable '%s' already exists and being overwritten in mapping.\n", name->chars);
  }

  mapping->count++;
  return value;
}
