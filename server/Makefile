# Simple recursive object build
CC      := gcc
CFLAGS  := -Wall -Wextra -Wfloat-equal -O2 -Iinclude

OBJDIR  := obj
BINDIR  := bin

# Find all .c files recursively (excluding obj/)
SRCS := $(shell find . -type f -name '*.c' -not -path './$(OBJDIR)/*')

# Turn path/to/file.c into obj/path/to/file.o 
OBJS := $(patsubst ./%.c,$(OBJDIR)/%.o,$(SRCS))

# Separate objects by directory
SUP_OBJS  := $(filter $(OBJDIR)/supervisor/%,$(OBJS))
ORC_OBJS  := $(filter $(OBJDIR)/orchestrator/%,$(OBJS))
GAME_OBJS := $(filter $(OBJDIR)/game/%,$(OBJS))

COMMON_OBJS := $(filter-out $(SUP_OBJS) $(ORC_OBJS) $(GAME_OBJS),$(OBJS))

SUP_BIN  := $(BINDIR)/supervisor/supervisor
ORC_BIN  := $(BINDIR)/orchestrator/orchestrator
GAME_BIN := $(BINDIR)/game/game

# Now make builds everything
all: $(SUP_BIN) $(ORC_BIN) $(GAME_BIN)

# Compile source files into object files
$(OBJDIR)/%.o: ./%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link each process 

$(SUP_BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(SUP_OBJS) $(COMMON_OBJS) -o $@

$(ORC_BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(ORC_OBJS) $(COMMON_OBJS) -o $@

$(GAME_BIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(GAME_OBJS) $(COMMON_OBJS) -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean