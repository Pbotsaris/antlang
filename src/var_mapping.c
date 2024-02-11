#include "var_mapping.h"
#include "strings.h"
#include "table.h"
#include "value.h"
#include <stdio.h>

void init_mapping(VarMapping *mapping);
void free_mapping(VarMapping *mapping);
Value add_mapping(VarMapping *mapping, ObjectString *name);
ObjectString *get_variable_name(VarMapping *mapping, int32_t index);


const VarMappingAPI ant_mapping = {
    .init = init_mapping,
    .free = free_mapping,
    .add = add_mapping,
    .find_name = get_variable_name,
};

void init_mapping(VarMapping *mapping) {
  mapping->count = 0;
  ant_table.init(&mapping->table);
  ant_value_array.init(&mapping->reverse_lookup);
}

void free_mapping(VarMapping *mapping) {
  ant_table.free(&mapping->table);
  ant_value_array.free(&mapping->reverse_lookup);
  mapping->count = -1;
}

Value add_mapping(VarMapping *mapping, ObjectString *name) {

   if(mapping->count == -1) {
    fprintf(stderr, "Error: Mapping is not initialized.\n");
    return ant_value.make_nil();
   }

   if(mapping->count != mapping->reverse_lookup.count) {
    fprintf(stderr, "Error: Mapping reverse lookup index mismatch: mapping: %d <> lookup: %d\n",
          mapping->count, mapping->reverse_lookup.count);

    return ant_value.make_nil();
   }

  Value index_value;
  bool found = ant_table.get(&mapping->table, name, &index_value);

  if (found) {
    return index_value;
  }

  index_value       = ant_value.from_number(mapping->count);
  bool is_new       = ant_table.set(&mapping->table, name, index_value);

  if (!is_new) {
    fprintf(stderr, "Warning: Variable '%s' being overwritten in mapping.\n", name->chars);
  }

  Value nameValue = ant_value.from_object(ant_string.as_object(name));
  ant_value_array.write(&mapping->reverse_lookup, nameValue);

  mapping->count++;
  return index_value;
}

ObjectString *get_variable_name(VarMapping *mapping, int32_t index){

   if(index < 0 || index >= mapping->count || index >= mapping->reverse_lookup.count) {
    fprintf(stderr, "Error: Invalid variable index: %d. mapping->count: %d, reverse_lookup->count: %d\n",
          index, mapping->count, mapping->reverse_lookup.count);

    return NULL;
   }

   Value nameValue = ant_value_array.at(&mapping->reverse_lookup, index);


   return ant_string.from_value(nameValue);
}

