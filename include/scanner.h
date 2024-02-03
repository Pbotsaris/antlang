#ifndef ANT_SCANNER_H
#define ANT_SCANNER_H
#include "common.h"
#include "token.h"
typedef struct {
   const char* start;
   const char* current;
   int32_t line;
}Scanner;

typedef struct AntScanner{
   Scanner* (*new)(void);
   void (*init)(Scanner *scanner, const char* source);
   void (*free)(Scanner* scanner);
   Token (*scan_token)(Scanner* scanner);
}AntScannerAPI;


extern AntScannerAPI ant_scanner;

#endif
