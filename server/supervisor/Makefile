CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE -Iinclude -I../common/include

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE -Iinclude -I../common/include
endif

SRC := main.c
OBJ := obj/main.o

all: dirs $(OBJ)

dirs:
	mkdir -p obj

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj

.PHONY: all clean dirs