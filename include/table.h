#ifndef ANT_TABLE_H
#define ANT_TABLE_H
#include "common.h"
#include "value.h"

typedef struct {
   ObjectString *key;
   Value value;
}Entry;


typedef struct {
   int32_t count;
   int32_t capacity;
   Entry *entries;
}Table;

typedef struct{
   void            (*init)   (Table *table);
   void            (*free)   (Table *table);
   bool            (*set)    (Table *table, ObjectString *key, Value value);
   bool            (*get)    (Table *table, ObjectString *key, Value *value);
   bool            (*delete) (Table *table, ObjectString *key);
   void            (*copy)   (Table *from, Table *to);
   ObjectString*   (*find)   (Table* table, const char* chars, int length, uint32_t hash);
   
}TableAPI;

extern TableAPI ant_table;
#endif
