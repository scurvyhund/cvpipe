# Master Makefile for String and GMP Arithmetic Libraries

.PHONY: all strarith gmparith test clean install

all: strarith gmparith

strarith:
	@echo "=== Building libstrarith.a (Pure C String Library) ==="
	$(MAKE) -C strarith
	@echo ""

gmparith:
	@echo "=== Building libgmparith.a (GMP Arithmetic Library) ==="
	$(MAKE) -C gmparith
	@echo ""

test: all
	@echo "=== Building test programs ==="
	$(MAKE) -C test
	@echo ""

clean:
	@echo "=== Cleaning all libraries ==="
	$(MAKE) -C strarith clean
	$(MAKE) -C gmparith clean
	@if [ -d test ]; then $(MAKE) -C test clean; fi
	@echo "Clean complete"

# Install libraries to parent directory (optional)
install: all
	@echo "=== Installing libraries ==="
	cp strarith/libstrarith.a ../
	cp strarith/strarith.h ../
	cp gmparith/libgmparith.a ../
	cp gmparith/gmparith.h ../
	@echo "Libraries installed to parent directory"

# Show library contents
list: all
	@echo "=== libstrarith.a contents ==="
	ar t strarith/libstrarith.a
	@echo ""
	@echo "=== libgmparith.a contents ==="
	ar t gmparith/libgmparith.a

# Show symbols
symbols: all
	@echo "=== libstrarith.a symbols ==="
	nm -g strarith/libstrarith.a | grep ' T '
	@echo ""
	@echo "=== libgmparith.a symbols ==="
	nm -g gmparith/libgmparith.a | grep ' T '

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build both libraries (default)"
	@echo "  strarith  - Build libstrarith.a only"
	@echo "  gmparith  - Build libgmparith.a only"
	@echo "  test      - Build test programs"
	@echo "  clean     - Remove all build artifacts"
	@echo "  install   - Copy libraries to parent directory"
	@echo "  list      - Show library contents"
	@echo "  symbols   - Show exported symbols"
	@echo "  help      - Show this help message"
