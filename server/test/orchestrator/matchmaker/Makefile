CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = entry.c client.c
OBJ = obj/entry.o obj/client.o

BIN = bin/test_client

all: $(BIN)

$(BIN): $(OBJ)
	mkdir -p bin
	$(CC) $(OBJ) -o $(BIN)

obj/%.o: %.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf obj bin

.PHONY: all run clean