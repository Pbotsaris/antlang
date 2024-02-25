SRC=src
OBJ=obj
BIN=bin

CFLAGS +=-W  -Wall -Wextra -g3 -Iinclude
LDFLAGS +=-fsanitize=address

PROFILE_FLAGS=-pg
OPTIMIZATION_FLAGS=-O2

CC=clang
TARGET=${BIN}/ant
VALGRIND_TARGET=${BIN}/ant_valgrind
PROFILE_TARGET=${BIN}/ant_profile

$(shell mkdir -p obj bin)

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

all: $(TARGET)

valgrind: $(VALGRIND_TARGET)
	# args are passed on via environment variable to make
	# make valgrind ARGS="your args here"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(VALGRIND_TARGET) $(ARGS)

valgrind-gdb: $(VALGRIND_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --vgdb=yes --vgdb-error=0 ./$(VALGRIND_TARGET) $(ARGS)

profile: $(PROFILE_TARGET)
	./$(PROFILE_TARGET) $(ARGS)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LDFLAGS)

$(VALGRIND_TARGET): $(OBJS)
	$(CC) -o $(VALGRIND_TARGET) $(OBJS) $(CFLAGS)

$(PROFILE_TARGET): $(OBJS)
	$(CC) -o $(PROFILE_TARGET) $(OBJS) $(PROFILE_FLAGS) $(OPTIMIZATION_FLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -rf $(OBJ)/*.o $(TARGET)

.PHONY: all clean run
