CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = entry.c client.c
OBJ = obj/entry.o obj/client.o

BIN = bin/test_client
SINGLE_PROC_BIN = bin/single-proc-test

all: $(BIN) $(SINGLE_PROC_BIN)

$(BIN): $(OBJ)
	mkdir -p bin
	$(CC) $(OBJ) -o $(BIN)

$(SINGLE_PROC_BIN): single-process-test.c test-config.h
	mkdir -p bin
	$(CC) $(CFLAGS) single-process-test.c -o $(SINGLE_PROC_BIN)

obj/%.o: %.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

run-single: $(SINGLE_PROC_BIN)
	./$(SINGLE_PROC_BIN)

clean:
	rm -rf obj bin

.PHONY: all run run-single clean