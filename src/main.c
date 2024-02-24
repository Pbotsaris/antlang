#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

static void run_file(VM *vm, const char *path);
static char *read_file(const char *path);

int main(int ac, char *av[]) {
  VM *vm = ant_vm.new();

  switch (ac) {
  case 1:
    ant_vm.repl(vm);
    break;
  case 2:
    run_file(vm, av[1]);
    break;
  default:
    fprintf(stderr, "Usage: ant [path]\n");
    break;
  }

  ant_vm.free(vm);
  return 0;
}

static void run_file(VM *vm, const char *path) {
  char *source = read_file(path);

  InterpretResult result = ant_vm.interpret(vm, source);

  if (result == INTERPRET_COMPILE_ERROR) printf("Compile error\n");

  free(source);
}

static char *read_file(const char *path) {

  FILE *file = fopen(path, "rb");

  if (file == NULL) {
    printf("Error: Could not open file: %s\n", path);
    exit(74);
  }

  int r = fseek(file, 0L, SEEK_END);

  if (r != 0) {
    printf("Error: Could not seek file: %s\n", path);
    exit(74);
  }

  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc((file_size + 1) * sizeof(char));

  if (buffer == NULL) {
    printf("Error: Could not allocate memory for reading file: %s\n", path);
    exit(74);
  }

  size_t rd_len = fread(buffer, sizeof(char), file_size, file);

  if (rd_len < file_size) {
    printf("Error: Could not read file: %s. File size: %ld bytes, read: %ld " "bytes\n", path, file_size, rd_len);
    exit(74);
  }

  buffer[rd_len] = '\0';

  fclose(file);
  return buffer;
}
