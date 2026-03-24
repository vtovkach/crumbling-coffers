CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

OBJ_DIR = obj
BIN_DIR = bin

SRC     = init_test.c
OBJ     = $(OBJ_DIR)/init_test.o
TARGET  = $(BIN_DIR)/init_test

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(OBJ): $(SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)