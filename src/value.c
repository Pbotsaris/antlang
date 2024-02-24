#include "value.h"
#include "object.h"
#include <stdio.h>


static Value value_from_bool(bool value);
static Value value_from_number(double value);
static Value value_from_object(Object *value);
static Value value_from_nil();
static Value value_from_undefined(void);

static bool value_to_bool(Value value);
static double value_to_number(Value value);
static Object *value_to_object(Value value);

static bool is_bool(Value value);
static bool is_number(Value value);
static bool is_nil(Value value);
static bool is_undefined(Value value);
static bool is_object(Value value);

static int32_t print_value(Value value, bool debug);

static Value equals(Value a, Value b);
static Value is_falsey(Value value);
static bool is_falsey_bool(Value value);


ValueAPI ant_value = {.from_bool = value_from_bool,
                      .from_number = value_from_number,
                      .from_object = value_from_object,
                      .make_nil = value_from_nil,
                      .make_undefined = value_from_undefined,
                      .as_bool = value_to_bool,
                      .as_number = value_to_number,
                      .as_object = value_to_object,
                      .is_bool = is_bool,
                      .is_number = is_number,
                      .is_undefined = is_undefined,
                      .is_nil = is_nil,
                      .is_object = is_object,
                      .print = print_value,
                      .equals = equals,
                      .is_falsey = is_falsey,
                      .is_falsey_bool = is_falsey_bool,
};

/* */
static Value value_from_bool(bool value) {
  return (Value){.type = VAL_BOOL, .as.boolean = value};
}

/* */

static Value value_from_number(double value) {
  return (Value){.type = VAL_NUMBER, .as.number = value};
}

/* */

static Value value_from_nil() {
  return (Value){.type = VAL_NIL, .as.number = 0};
}

/* */

static Value value_from_undefined(void) {
  return (Value){.type = VAL_UNDEFINED, .as.number = 0};
}

/* */

static Value value_from_object(Object *value) {
  return (Value){.type = VAL_OBJECT, .as.object = value};
}

/* */
static bool value_to_bool(Value value) {
  if (value.type != VAL_BOOL) {
    fprintf(stderr, "Value is not a boolean. Returning default: false\n");
    return false;
  }

  return value.as.boolean;
}

/* */

static double value_to_number(Value value) {
  if (value.type != VAL_NUMBER) {
    fprintf(stderr, "Value is not a number. Returning default: 0\n");
    return 0;
  }

  return value.as.number;
}

/* */

static Object *value_to_object(Value value) {
  if (value.type != VAL_OBJECT) {
    fprintf(stderr, "Value is not an object. Returning default: NULL\n");
    return NULL;
  }

  return value.as.object;
}

/* */

static bool is_bool(Value value) { return value.type == VAL_BOOL; }
static bool is_number(Value value) { return value.type == VAL_NUMBER; }
static bool is_undefined(Value value) { return value.type == VAL_UNDEFINED; }
static bool is_nil(Value value) { return value.type == VAL_NIL; }
static bool is_object(Value value) { return value.type == VAL_OBJECT; }

/* */

static Value is_falsey(Value value) {
  bool boolean = is_nil(value) || (is_bool(value) && !value.as.boolean);
  return value_from_bool(boolean);
}

static bool is_falsey_bool(Value value) {
   return is_nil(value) || (is_bool(value) && !value.as.boolean);
}

/* */

static Value equals(Value a, Value b) {

  // If the types are different, they are not equal
  if (a.type != b.type) {
    return value_from_bool(false);
  }

  switch (a.type) {
  case VAL_NIL:
    return value_from_bool(true);

  case VAL_BOOL:
    return value_from_bool(a.as.boolean == b.as.boolean);

  case VAL_NUMBER:
    return value_from_bool(a.as.number == b.as.number);

  case VAL_OBJECT:
    /* we can do this because our strings are all internalized
     * So we can compare memory addreses
     */
    return value_from_bool(a.as.object == b.as.object);

  default:
    return value_from_bool(false); /* unreachable */
  }
}

/* */

static int32_t print_value(Value value, bool debug) {
  switch (value.type) {
  case VAL_BOOL:
    return printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VAL_NIL:
    return printf("nil");
    break;
  case VAL_NUMBER:
    return printf("%g", value.as.number);
    break;
  case VAL_UNDEFINED:
    return printf("Undefined");
    break;
  case VAL_OBJECT:
    return ant_object.print(value, debug);
    break;
  }

  return 0;
}
