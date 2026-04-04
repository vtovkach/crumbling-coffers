CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE
endif

BIN  := bin/supervisor
OBJS  = $(wildcard common/obj/*.o) $(wildcard supervisor/obj/*.o)

all: common supervisor link

common:
	$(MAKE) -C common

supervisor:
	$(MAKE) -C supervisor

link:
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) -lpthread

run: all
	./$(BIN)

clean:
	$(MAKE) -C common clean
	$(MAKE) -C supervisor clean
	rm -rf bin

.PHONY: all common supervisor link run clean
