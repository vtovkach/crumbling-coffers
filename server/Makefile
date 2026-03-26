CC := gcc

SUPERVISOR_BIN := bin/supervisor
GAME_BIN       := bin/game
ORCH_BIN       := bin/orch

TEST_DIR_MATCHMAKER := test/orchestrator/matchmaker
TEST_DIR_GAME := test/game/init-test
TEST_DIR_NET_T := test/game/net-thread-test

all: modules $(GAME_BIN) $(SUPERVISOR_BIN) $(ORCH_BIN)

modules:
	$(MAKE) -C common
	$(MAKE) -C game
	$(MAKE) -C orchestrator
	$(MAKE) -C supervisor

$(GAME_BIN): modules
	@mkdir -p bin
	$(CC) \
		$$(find game/obj -type f -name '*.o') \
		$$(find common/obj -type f -name '*.o') \
		-o $@ \
		-pthread

$(SUPERVISOR_BIN): modules
	@mkdir -p bin
	$(CC) \
		$$(find supervisor/obj -type f -name '*.o') \
		$$(find common/obj -type f -name '*.o') \
		-o $@

$(ORCH_BIN): modules
	@mkdir -p bin
	$(CC) \
		$$(find orchestrator/obj -type f -name '*.o') \
		$$(find common/obj -type f -name '*.o') \
		-o $@ \
		-pthread
	
run:
	./$(SUPERVISOR_BIN)

clean:
	$(MAKE) -C common clean
	$(MAKE) -C game clean
	$(MAKE) -C orchestrator clean
	$(MAKE) -C supervisor clean
	$(MAKE) -C $(TEST_DIR) clean
	rm -rf bin

# Tests
test:
	$(MAKE) -C $(TEST_DIR_MATCHMAKER)
	$(MAKE) -C $(TEST_DIR_GAME)
	$(MAKE) -C $(TEST_DIR_NET_T)

test-clean:
	$(MAKE) -C $(TEST_DIR_MATCHMAKER) clean
	$(MAKE) -C $(TEST_DIR_GAME) clean 
	$(MAKE) -C $(TEST_DIR_NET_T) clean 

test-run-matchmaker: test
	$(MAKE) -C $(TEST_DIR_MATCHMAKER) run

test-run-game:
	$(MAKE) -C $(TEST_DIR_GAME) run

test-run-net:
	$(MAKE) -C $(TEST_DIR_NET_T) run

.PHONY: test test-clean test-run