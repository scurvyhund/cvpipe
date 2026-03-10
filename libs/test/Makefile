# Makefile for library test programs

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11

# Library paths
STRARITH_LIB = ../strarith/libstrarith.a
GMPARITH_LIB = ../gmparith/libgmparith.a

# Test programs
TESTS = test_strarith test_gmparith

all: $(TESTS)

test_strarith: test_strarith.c $(STRARITH_LIB)
	$(CC) $(CFLAGS) $< -L../strarith -lstrarith -o $@
	@echo "Built $@"

test_gmparith: test_gmparith.c $(GMPARITH_LIB)
	$(CC) $(CFLAGS) $< -L../gmparith -lgmparith -lgmp -o $@
	@echo "Built $@"

# Run all tests
run: all
	@echo ""
	@echo "════════════════════════════════════════════════════════════════"
	@echo "  Running libstrarith.a tests"
	@echo "════════════════════════════════════════════════════════════════"
	@./test_strarith
	@echo ""
	@echo "════════════════════════════════════════════════════════════════"
	@echo "  Running libgmparith.a tests"
	@echo "════════════════════════════════════════════════════════════════"
	@./test_gmparith
	@echo ""

clean:
	rm -f $(TESTS)

.PHONY: all run clean
