# Simple recursive object build
CC      := gcc
CFLAGS  := -Wall -Wextra -Wfloat-equal -O2 -Iinclude

OBJDIR  := obj

# Find all .c files recursively (excluding obj/)
SRCS := $(shell find . -type f -name '*.c' -not -path './$(OBJDIR)/*')

# Turn path/to/file.c into obj/path/to/file.o 
OBJS := $(patsubst ./%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean
all: $(OBJS)

# - creates the matching obj/ with corresponding subdirectory of the source file 
# - just create object files for now 
$(OBJDIR)/%.o: ./%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)