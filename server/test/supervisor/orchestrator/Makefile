CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE -I../../../common/include -I..

SRC := $(wildcard *.c)
OBJ := $(addprefix obj/, $(notdir $(SRC:.c=.o)))

all: dirs $(OBJ)

dirs:
	mkdir -p obj

obj/%.o: %.c ../test-config.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj

.PHONY: all dirs clean
