CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE -Iinclude -I../common/include

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE -Iinclude -I../common/include
endif

SRC := $(wildcard *.c src/*.c src/orchestrator/*.c src/matchmaker/*.c)
OBJ := $(addprefix obj/, $(notdir $(SRC:.c=.o)))

all: dirs $(OBJ)

dirs:
	mkdir -p obj

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/orchestrator/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/matchmaker/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj

.PHONY: all clean dirs