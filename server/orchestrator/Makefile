CC      := gcc
CFLAGS  := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE -Iinclude -I../common/include -std=c23

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE -Iinclude -I../common/include -std=c23
endif

SRC_DIR := src
OBJ_DIR := obj

SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

all: dirs $(OBJS)

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all dirs clean