CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE
endif

BIN      := bin/supervisor
OBJS      = $(wildcard common/obj/*.o) $(wildcard supervisor/obj/*.o)

TEST_BIN         := bin/test_orch_input
TEST_OBJS        := test/supervisor/orchestrator/obj/test_input.o

TEST_CHUNKS_BIN  := bin/test_orch_input_chunks
TEST_CHUNKS_OBJS := test/supervisor/orchestrator/obj/test_input_chunks.o

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

test: all test-build link-test

test-build:
	$(MAKE) -C test/supervisor/orchestrator

link-test:
	mkdir -p bin
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_BIN)
	$(CC) $(CFLAGS) $(TEST_CHUNKS_OBJS) -o $(TEST_CHUNKS_BIN)

test-run: test
	@FIFO=$$(mktemp -u /tmp/sv_stdin_XXXXXX); \
	mkfifo $$FIFO; \
	./$(BIN) < $$FIFO & SPID=$$!; \
	exec 3>$$FIFO; \
	./$(TEST_BIN); R1=$$?; \
	./$(TEST_CHUNKS_BIN); R2=$$?; \
	echo exit >&3; \
	exec 3>&-; \
	wait $$SPID; \
	rm -f $$FIFO; \
	[ $$R1 -eq 0 ] && [ $$R2 -eq 0 ]

clean:
	$(MAKE) -C common clean
	$(MAKE) -C supervisor clean
	$(MAKE) -C test/supervisor/orchestrator clean
	rm -rf bin

.PHONY: all common supervisor link run test test-build link-test test-run clean
