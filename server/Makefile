all:
	$(MAKE) -C common
	$(MAKE) -C game
	$(MAKE) -C supervisor

run:
	$(MAKE) -C supervisor run

clean:
	$(MAKE) -C common clean
	$(MAKE) -C game clean
	$(MAKE) -C supervisor clean

.PHONY: all run clean
