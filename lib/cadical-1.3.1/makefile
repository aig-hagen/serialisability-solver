CADICALBUILD=/home/lars/Documents/GitHub/UnchallengedSemantics/c++/lib/cadical-1.3.1/build
all:
	$(MAKE) -C "$(CADICALBUILD)"
clean:
	@if [ -d "$(CADICALBUILD)" ]; \
	then \
	  if [ -f "$(CADICALBUILD)"/makefile ]; \
	  then \
	     touch "$(CADICALBUILD)"/build.hpp; \
	     $(MAKE) -C "$(CADICALBUILD)" clean; \
	  fi; \
	  rm -rf "$(CADICALBUILD)"; \
	fi
	rm -f "/home/lars/Documents/GitHub/UnchallengedSemantics/c++/lib/cadical-1.3.1/makefile"
test:
	$(MAKE) -j1 -C "$(CADICALBUILD)" test
cadical:
	$(MAKE) -j1 -C "$(CADICALBUILD)" cadical
mobical:
	$(MAKE) -j1 -C "$(CADICALBUILD)" mobical
update:
	$(MAKE) -j1 -C "$(CADICALBUILD)" update
.PHONY: all cadical clean mobical test update
