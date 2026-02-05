# Makefile for CVPipe - Con-verse Prime Hunting Pipeline
# GMP-powered, unlimited range
# Optimized for AMD Ryzen 7 4700U

CC = gcc
CFLAGS = -O3 -march=znver2 -mtune=znver2 -fopenmp -Wall
LDFLAGS = -lgmp

# All executables (with _gmp suffix for clarity)
STAGE1 = generate_candidates_gmp
STAGE2 = filter_primes_gmp
STAGE3 = check_emirp_gmp
STAGE35 = check_palindrome_gmp
STAGE4 = check_converse_gmp
STAGE5 = gen_candidates_range_gmp
CVPIPE = cvpipe

ALL = $(STAGE1) $(STAGE2) $(STAGE3) $(STAGE35) $(STAGE4) $(STAGE5) $(CVPIPE)

# Default: build everything
all: $(ALL)
	@echo ""
	@echo "CVPipe Build Complete - GMP Edition"
	@echo "======================================================================"
	@echo "All stages built with GMP (unlimited range)"
	@echo ""
	@echo "Stage 1:   ./$(STAGE1) <max_prime>"
	@echo "Stage 2:   ./$(STAGE2)"
	@echo "Stage 3:   ./$(STAGE3)"
	@echo "Stage 3.5: ./$(STAGE35)"
	@echo "Stage 4:   ./$(STAGE4)"
	@echo "Stage 5:   ./$(STAGE5) <start> <end>"
	@echo ""
	@echo "Merged:    ./$(CVPIPE) <max_prime>"
	@echo "           ./$(CVPIPE) <start_n> <max_prime>"
	@echo ""
	@echo "Or use: make test / make run / make extreme"
	@echo "Or use: make 10e21 / make 10e22 / make 10e23 / make 10e24"
	@echo "Or use: make continue_10e24"
	@echo ""
	@echo "Merged pipeline: make cvpipe-test / make cvpipe-10e20 ... cvpipe-10e25"
	@echo "======================================================================"
	@echo ""

# Individual stages
$(STAGE1): generate_candidates_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 1 - GMP)"

$(STAGE2): filter_primes_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 2 - GMP)"

$(STAGE3): check_emirp_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 3 - GMP)"

$(STAGE35): check_palindrome_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 3.5 - GMP)"

$(STAGE4): check_converse_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 4 - GMP)"

$(STAGE5): gen_candidates_range_gmp.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Stage 5 - Incremental Range Generator - GMP)"

$(CVPIPE): cvpipe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Built: $@ (Merged Pipeline - GMP)"

# Quick test - small range
test: $(ALL)
	@echo "Running Quick Test (max_prime = 100 million)"
	@echo "======================================================================"
	@./$(STAGE1) 100000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "Test complete - check results above"

# Standard run - to 3 billion (near uint64_t limit)
run: $(ALL)
	@echo "Running Standard Search (n ≈ 3 billion)"
	@echo "======================================================================"
	@./$(STAGE1) 20000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "Search complete - check converse.dat"

# Extreme run - beyond uint64_t!
extreme: $(ALL)
	@echo "Running EXTREME Search (10^20 - breaking 2^64 barrier!)"
	@echo "WARNING: This may take HOURS"
	@echo "======================================================================"
	@echo "Press Ctrl+C within 5 seconds to cancel..."
	@sleep 5
	@./$(STAGE1) 100000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "EXTREME search complete!"

# Dedicated power-of-10 targets (NOHUP-FRIENDLY!)
10e20: $(ALL)
	@echo "Running 10^20 search"
	@echo "======================================================================"
	@./$(STAGE1) 100000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^20 complete!"

10e21: $(ALL)
	@echo "Running 10^21 search (~10 hours)"
	@echo "======================================================================"
	@./$(STAGE1) 1000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^21 complete!"

10e22: $(ALL)
	@echo "Running 10^22 search (~100 hours)"
	@echo "======================================================================"
	@./$(STAGE1) 10000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^22 complete!"

10e23: $(ALL)
	@echo "Running 10^23 search (~1000 hours)"
	@echo "======================================================================"
	@./$(STAGE1) 100000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^23 complete!"

10e24: $(ALL)
	@echo "Running 10^24 search (VERY LONG - weeks!)"
	@echo "======================================================================"
	@./$(STAGE1) 1000000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^24 complete!"

10e25: $(ALL)
	@echo "Running 10^25 search (EXTREMELY LONG - months!)"
	@echo "======================================================================"
	@./$(STAGE1) 10000000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^25 complete!"

# Incremental continuation from 10^23 to 10^24
continue_10e24: $(STAGE5) $(STAGE2) $(STAGE3) $(STAGE35) $(STAGE4)
	@echo "Continuing from 10^23 to 10^24"
	@echo "======================================================================"
	@./$(STAGE5) 100000000000000000000000 1000000000000000000000000
	@./$(STAGE2)
	@./$(STAGE3)
	@./$(STAGE35)
	@./$(STAGE4)
	@echo ""
	@echo "10^23 → 10^24 continuation complete!"

# ── Merged pipeline (cvpipe) targets ──────────────────────────────────
cvpipe-test: $(CVPIPE)
	@echo "Running CVPipe Quick Test (max_prime = 100 million)"
	@echo "======================================================================"
	@./$(CVPIPE) 100000000
	@echo ""
	@echo "Test complete - check converse.dat and otto_primes.dat"

cvpipe-10e20: $(CVPIPE)
	@echo "CVPipe: 10^20 search"
	@echo "======================================================================"
	@./$(CVPIPE) 100000000000000000000

cvpipe-10e21: $(CVPIPE)
	@echo "CVPipe: 10^21 search"
	@echo "======================================================================"
	@./$(CVPIPE) 1000000000000000000000

cvpipe-10e22: $(CVPIPE)
	@echo "CVPipe: 10^22 search"
	@echo "======================================================================"
	@./$(CVPIPE) 10000000000000000000000

cvpipe-10e23: $(CVPIPE)
	@echo "CVPipe: 10^23 search"
	@echo "======================================================================"
	@./$(CVPIPE) 100000000000000000000000

cvpipe-10e24: $(CVPIPE)
	@echo "CVPipe: 10^24 search"
	@echo "======================================================================"
	@./$(CVPIPE) 1000000000000000000000000

cvpipe-10e25: $(CVPIPE)
	@echo "CVPipe: 10^25 search"
	@echo "======================================================================"
	@./$(CVPIPE) 10000000000000000000000000

# Manual stages (for step-by-step control)
stage1: $(STAGE1)
	@echo "Stage 1 built. Run: ./$(STAGE1) <max_prime>"

stage2: $(STAGE2)
	@echo "Stage 2 built. Run: ./$(STAGE2)"

stage3: $(STAGE3)
	@echo "Stage 3 built. Run: ./$(STAGE3)"

stage3.5: $(STAGE35)
	@echo "Stage 3.5 built. Run: ./$(STAGE35)"

stage4: $(STAGE4)
	@echo "Stage 4 built. Run: ./$(STAGE4)"

stage5: $(STAGE5)
	@echo "Stage 5 built. Run: ./$(STAGE5) <start> <end>"

# Verify GMP installation
check-gmp:
	@echo "Checking GMP installation..."
	@pkg-config --modversion gmp 2>/dev/null && \
		echo "GMP version: $$(pkg-config --modversion gmp)" || \
		echo "GMP not found - install with: sudo dnf install gmp-devel"

# System info
info:
	@echo "System Information"
	@echo "======================================================================"
	@echo "CPU:         AMD Ryzen 7 4700U (Zen 2)"
	@echo "Cores:       8 physical"
	@echo "RAM:         64 GB"
	@echo "Compiler:    $(CC)"
	@$(CC) --version | head -1
	@echo "Flags:       $(CFLAGS)"
	@echo "======================================================================"
	@pkg-config --modversion gmp 2>/dev/null | sed 's/^/GMP version: /' || echo "GMP: Not found"
	@echo "======================================================================"

# Clean binaries
clean:
	rm -f $(ALL)
	@echo "Cleaned executables"

# Clean everything (binaries + data files)
distclean: clean
	rm -f p*.dat primes*.dat emirps.dat otto_primes.dat converse.dat
	@echo "Deep cleaned (including data files)"

# Help
help:
	@echo "CVPipe Makefile - Con-verse Prime Hunter"
	@echo "======================================================================"
	@echo "BUILD TARGETS:"
	@echo "  make              Build all stages"
	@echo "  make stage1       Build Stage 1 only"
	@echo "  make stage2       Build Stage 2 only"
	@echo "  make stage3       Build Stage 3 only"
	@echo "  make stage3.5     Build Stage 3.5 only"
	@echo "  make stage4       Build Stage 4 only"
	@echo "  make stage5       Build Stage 5 only (incremental range)"
	@echo ""
	@echo "RUN TARGETS:"
	@echo "  make test         Quick test (100M, ~1 minute)"
	@echo "  make run          Standard (2e19, ~10 minutes)"
	@echo "  make extreme      10^20 search (~1 hour)"
	@echo ""
	@echo "POWER-OF-10 TARGETS (nohup-friendly!):"
	@echo "  make 10e20        10^20 (~1 hour)"
	@echo "  make 10e21        10^21 (~10 hours)"
	@echo "  make 10e22        10^22 (~100 hours)"
	@echo "  make 10e23        10^23 (~1000 hours)"
	@echo "  make 10e24        10^24 (weeks!)"
	@echo "  make 10e25        10^25 (months!)"
	@echo ""
	@echo "INCREMENTAL CONTINUATION:"
	@echo "  make continue_10e24   Continue from 10^23 to 10^24"
	@echo ""
	@echo "UTILITY TARGETS:"
	@echo "  make info         Show system/compiler info"
	@echo "  make check-gmp    Verify GMP installation"
	@echo "  make clean        Remove binaries"
	@echo "  make distclean    Remove binaries + data files"
	@echo "  make help         Show this help"
	@echo ""
	@echo "MANUAL USAGE:"
	@echo "  ./$(STAGE1) <max_prime>"
	@echo "  ./$(STAGE2)"
	@echo "  ./$(STAGE3)"
	@echo "  ./$(STAGE35)"
	@echo "  ./$(STAGE4)"
	@echo "  ./$(STAGE5) <start> <end>"
	@echo ""
	@echo "NOHUP EXAMPLES:"
	@echo "  nohup time make 10e21 > run_10e21.log 2>&1 &"
	@echo "  nohup time make 10e22 > run_10e22.log 2>&1 &"
	@echo "  nohup time make 10e23 > run_10e23.log 2>&1 &"
	@echo "  nohup time make continue_10e24 > run_10e23_to_10e24.log 2>&1 &"
	@echo "======================================================================"

.PHONY: all test run extreme 10e20 10e21 10e22 10e23 10e24 10e25 \
        continue_10e24 stage1 stage2 stage3 stage3.5 stage4 stage5 \
        cvpipe-test cvpipe-10e20 cvpipe-10e21 cvpipe-10e22 \
        cvpipe-10e23 cvpipe-10e24 cvpipe-10e25 \
        check-gmp info clean distclean help
