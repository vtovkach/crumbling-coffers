CC     := gcc
CFLAGS := -Wall -Wextra -Wfloat-equal -O2 -D_GNU_SOURCE

ifdef DEBUG
CFLAGS := -Wall -Wextra -Wfloat-equal -O0 -g -D_GNU_SOURCE
endif

BIN      := bin/supervisor
OBJS      = $(wildcard common/obj/*.o) $(wildcard supervisor/obj/*.o)

GAME_BIN  := bin/game
GAME_OBJS  = $(wildcard common/obj/*.o) $(wildcard game/obj/*.o)

TEST_BIN         := bin/test_orch_input
TEST_OBJS        := test/supervisor/orchestrator/obj/test_input.o

TEST_CHUNKS_BIN  := bin/test_orch_input_chunks
TEST_CHUNKS_OBJS := test/supervisor/orchestrator/obj/test_input_chunks.o

TEST_MATCHMAKING_BIN  := bin/test_matchmaking
TEST_MATCHMAKING_OBJS := test/supervisor/matchmaker/obj/test_matchmaking.o

TEST_MATCHMAKING_QUEUE_BIN  := bin/test_matchmaking_queue
TEST_MATCHMAKING_QUEUE_OBJS := test/supervisor/matchmaker/obj/test_matchmaking_queue.o

TEST_PORT_REAPER_BIN  := bin/test_port_reaper
TEST_PORT_REAPER_OBJS := test/supervisor/matchmaker/obj/test_port_reaper.o \
                         supervisor/obj/port_manager.o \
                         common/obj/hashmap.o \
                         common/obj/spsc_queue.o \
                         common/obj/log_system.o \
                         common/obj/util.o

all: common supervisor game link link-game

common:
	$(MAKE) -C common

supervisor:
	$(MAKE) -C supervisor

game:
	$(MAKE) -C game

link:
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) -lpthread

link-game:
	mkdir -p bin
	$(CC) $(CFLAGS) $(GAME_OBJS) -o $(GAME_BIN) -lpthread

run: all
	./$(BIN)

test: all test-build link-test

test-build: test-build-orchestrator test-build-matchmaker test-build-port-reaper

test-build-orchestrator:
	$(MAKE) -C test/supervisor/orchestrator

test-build-matchmaker:
	$(MAKE) -C test/supervisor/matchmaker

test-build-port-reaper:
	$(MAKE) -C test/supervisor/matchmaker

link-test: link-test-orchestrator link-test-matchmaker link-test-port-reaper

link-test-orchestrator: test-build-orchestrator
	mkdir -p bin
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_BIN)
	$(CC) $(CFLAGS) $(TEST_CHUNKS_OBJS) -o $(TEST_CHUNKS_BIN)

link-test-matchmaker: test-build-matchmaker
	mkdir -p bin
	$(CC) $(CFLAGS) $(TEST_MATCHMAKING_OBJS) -o $(TEST_MATCHMAKING_BIN)

link-test-matchmaking-queue: test-build-matchmaker
	mkdir -p bin
	$(CC) $(CFLAGS) $(TEST_MATCHMAKING_QUEUE_OBJS) -o $(TEST_MATCHMAKING_QUEUE_BIN)

link-test-port-reaper: test-build-port-reaper
	mkdir -p bin
	$(CC) $(CFLAGS) $(TEST_PORT_REAPER_OBJS) -o $(TEST_PORT_REAPER_BIN) -lpthread

test-orchestrator: test
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

test-orchestrator-remote: link-test-orchestrator
	@./$(TEST_BIN); R1=$$?; \
	./$(TEST_CHUNKS_BIN); R2=$$?; \
	[ $$R1 -eq 0 ] && [ $$R2 -eq 0 ]

test-matchmaker: test
	@FIFO=$$(mktemp -u /tmp/sv_stdin_XXXXXX); \
	mkfifo $$FIFO; \
	./$(BIN) < $$FIFO & SPID=$$!; \
	exec 3>$$FIFO; \
	./$(TEST_MATCHMAKING_BIN); R1=$$?; \
	echo exit >&3; \
	exec 3>&-; \
	wait $$SPID; \
	rm -f $$FIFO; \
	[ $$R1 -eq 0 ]

test-matchmaker-remote: link-test-matchmaker
	@./$(TEST_MATCHMAKING_BIN)

test-matchmaking-queue-remote: link-test-matchmaking-queue
	@./$(TEST_MATCHMAKING_QUEUE_BIN)

test-port-reaper: all test-build-port-reaper link-test-port-reaper
	@./$(TEST_PORT_REAPER_BIN)

clean:
	$(MAKE) -C common clean
	$(MAKE) -C supervisor clean
	$(MAKE) -C game clean
	$(MAKE) -C test/supervisor/orchestrator clean
	$(MAKE) -C test/supervisor/matchmaker clean
	rm -f $(TEST_PORT_REAPER_BIN)
	rm -rf bin

.PHONY: all common supervisor game link link-game run test \
        test-build test-build-orchestrator test-build-matchmaker test-build-port-reaper \
        link-test link-test-orchestrator link-test-matchmaker link-test-port-reaper \
        test-orchestrator test-orchestrator-remote \
        test-matchmaker test-matchmaker-remote \
        link-test-matchmaking-queue test-matchmaking-queue-remote \
        test-port-reaper clean
