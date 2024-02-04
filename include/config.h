#ifndef ANT_CONFIG_H
#define ANT_CONFIG_H

#define ANT_VERSION "0.1.0"

/* Debugging */
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
#define DEBUG_TRACE_PARSER
// Requires DEBUG_TRACE_PARSER
// will trace tokens
#define DEBUG_TRACE_PARSER_VERBOSE 

/* Options */
#define OPTION_STACK_MAX 256
#define OPTION_LINE_MAX 1024
#define OPTION_ARRAY_GROW_FACTOR 2
#define OPTION_ARRAY_MIN_CAPACITY 8
#define OPTION_TABLE_LOAD_FACTOR 0.79 // Load factor for hash table

/* Constants */
#define CONST_24BITS 3
#define CONST_8BITS 1
#define CONST_MAX_8BITS_VALUE 256
#define CONST_MAX_24BITS_VALUE 16777216


#endif // ANT_CONFIG_H
