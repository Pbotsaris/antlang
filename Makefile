SRC=src
OBJ=obj
BIN=bin

# Base
BASE_CFLAGS=-W -Wall -Wextra -Iinclude

# Debug 
DEBUG_CFLAGS=$(BASE_CFLAGS) -g3 -DDEBUG_TRACE_EXECUTION -DDEBUG_PRINT_CODE
DEBUG_VERBOSE_CFLAGS=$(DEBUG_CFLAGS) -DDEBUG_TRACE_PARSER -DDEBUG_TRACE_PARSER_VERBOSE 
DEBUG_LDFLAGS=-fsanitize=address

# Release
RELEASE_CFLAGS=$(BASE_CFLAGS) -O2

CC=clang
TARGET=${BIN}/ant
TARGET_DEBUG=${BIN}/ant_debug
VALGRIND_TARGET=${BIN}/ant_valgrind
PROFILE_TARGET=${BIN}/ant_profile

$(shell mkdir -p obj bin)

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

all: CFLAGS=$(RELEASE_CFLAGS)
all: $(TARGET)

debug: CFLAGS=$(DEBUG_CFLAGS)
debug: LDFLAGS=$(DEBUG_LDFLAGS)
debug: $(TARGET_DEBUG)

debug-verbose: CFLAGS=$(DEBUG_VERBOSE_CFLAGS)
debug-verbose: LDFLAGS=$(DEBUG_LDFLAGS)
debug-verbose: $(TARGET_DEBUG)


valgrind: CFLAGS=$(BASE_CFLAGS)
valgrind: $(VALGRIND_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(VALGRIND_TARGET) $(ARGS)

valgrind-gdb: $(VALGRIND_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --vgdb=yes --vgdb-error=0 ./$(VALGRIND_TARGET) $(ARGS)

profile: CFLAGS=$(RELEASE_CFLAGS) -pg
profile: $(PROFILE_TARGET)
	./$(PROFILE_TARGET) $(ARGS); gprof $(PROFILE_TARGET) gmon.out > analysis.txt

run: $(TARGET_DEBUG)
	./$(TARGET_DEBUG) $(ARGS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(TARGET_DEBUG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_DEBUG) $(OBJS)

$(VALGRIND_TARGET): $(OBJS)
	$(CC) $(DEBUG_CFLAGS) -o $(VALGRIND_TARGET) $(OBJS)

$(PROFILE_TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROFILE_TARGET) $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ)/*.o $(BIN)/*

.PHONY: all clean run debug valgrind valgrind-gdb profile
