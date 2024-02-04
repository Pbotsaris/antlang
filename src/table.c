#include "table.h"
#include "config.h"
#include "memory.h"
#include "strings.h"
#include <string.h>

static void init_table(Table *table);
static void free_table(Table *table);
static bool table_set(Table *table, ObjectString *key, Value value);
static bool table_get(Table *table, ObjectString *key, Value *value);
static bool table_delete(Table *table, ObjectString *key);
static void copy_table(Table *from, Table *to);
static ObjectString *find_key(Table *table, const char *chars, int32_t length, uint32_t hash);

static void adjust_capacity(Table *table, int32_t capacity);

/* Entries */
static Entry *find_entry(Entry *entries, int32_t capacity, ObjectString *key);

TableAPI ant_table = {
    .init = init_table,
    .free = free_table,
    .set = table_set,
    .get = table_get,
    .copy = copy_table,
    .delete = table_delete,
    .find = find_key,
    
};

static void init_table(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

static void free_table(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}

static bool table_set(Table *table, ObjectString *key, Value value) {

  if (table->count + 1 > table->capacity * OPTION_TABLE_LOAD_FACTOR) {
    int32_t capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);

  /* check if value is nil to not increment count when re-using a tombstone
   * value */
  bool is_new = entry->key == NULL && ant_value.is_nil(entry->value);
  if (is_new)
    table->count++;

  entry->key = key;
  entry->value = value;

  return is_new;
}

static bool table_get(Table *table, ObjectString *key, Value *value) {
  if (table->count == 0)
    return false; // Empty table

  Entry *entry = find_entry(table->entries, table->capacity, key);

  if (entry->key == NULL)
    return false;

  *value = entry->value;
  return true;
}

static bool table_delete(Table *table, ObjectString *key) {
  if (table->count == 0)
    return false;

  Entry *entry = find_entry(table->entries, table->capacity, key);

  if (entry->key == NULL)
    return false;

  /* Mark the entry as deleted with a "tombstone value"
   * to avoid braking the probing sequence
   * We don't decrement the count as we are not actually removing the entry
   */

  entry->key = NULL;
  entry->value = ant_value.make_bool(true);

  return true;
}

static void copy_table(Table *from, Table *to) {

  // NOTE: Don't need to check capacity as the new table will be resized

  for (int32_t i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];

    if (entry->key == NULL)
      continue;

    table_set(to, entry->key, entry->value);
  }
}

static ObjectString *find_key(Table *table, const char *chars, int32_t length,
                              uint32_t hash) {
  if (table->count == 0)
    return NULL;

  uint32_t index = hash % table->capacity;

  while (true) {
    Entry *entry =  &table->entries[index];

    /* Not found */
    if (entry->key == NULL && ant_value.is_nil(entry->value)) {
      return NULL;
    }

    bool found = entry->key->length == length &&
                 entry->key->hash == hash && 
                 memcmp(entry->key->chars, chars, length) == 0;

    if (found) {
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

static Entry *find_entry(Entry *entries, int32_t capacity, ObjectString *key) {

  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;

  while (true) {
    Entry *entry = &entries[index];

    // NOTE: this comparison is valid because ant interns ALL its strings
    if (entry->key == key) {
      return entry;
    }

    if (entry->key == NULL) {
      /*  found nil key and value, return the empty entry or a tombstone any was
       * found */
      if (ant_value.is_nil(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      }

      /* We found a tombstone value, assign for next iteration
       * But we keep going as the don't want the probing sequence to break
       */
      if (ant_value.is_bool(entry->value)) {
        if (tombstone == NULL)
          tombstone = entry;
      }
    }

    index = (index + 1) % capacity;
  }
}

static void adjust_capacity(Table *table, int32_t new_capacity) {
  Entry *new_entries = ALLOCATE(Entry, new_capacity);

  for (int32_t i = 0; i < new_capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = ant_value.make_nil();
  }

  /* count again as we discart tombstone entries during grow */
  table->count = 0;

  for (int32_t i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];

    /* tombstone values will be also discarted */
    if (entry->key == NULL) {
      continue;
    }

    Entry *dest = find_entry(new_entries, new_capacity, entry->key);

    /* Copy the old entries to the new table */
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++; // re-count the entries
  }
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = new_entries;
  table->capacity = new_capacity;
}
