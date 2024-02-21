SRC=src
OBJ=obj
BIN=bin

CFLAGS +=-W  -Wall -Wextra -g3 -Iinclude
LDFLAGS +=-fsanitize=address

CC=clang
TARGET=${BIN}/ant
VALGRIND_TARGET=${BIN}/ant_valgrind

$(shell mkdir -p obj bin)

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

all: $(TARGET)

valgrind: $(VALGRIND_TARGET)
	# args are passed on via environment variable to make
	# make valgrind ARGS="your args here"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(VALGRIND_TARGET) $(ARGS)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS) $(LDFLAGS)

$(VALGRIND_TARGET): $(OBJS)
	$(CC) -o $(VALGRIND_TARGET) $(OBJS) $(CFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -rf $(OBJ)/*.o $(TARGET)

.PHONY: all clean run
