CC      := gcc
CFLAGS  := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE -Iinclude

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE -Iinclude -I../common/include
endif

SRC_DIR := src
OBJ_DIR := obj

SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(SRCS:.c=.o)))

vpath %.c $(sort $(dir $(SRCS)))

all: dirs $(OBJS)

dirs:
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all dirs clean