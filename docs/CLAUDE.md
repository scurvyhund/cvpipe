# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CVPipe (Converse Prime Pipeline) is a C program that searches for converse primes — an extremely rare class of prime pairs. A converse prime pair (p, q) requires: both p and q are prime, q = reverse(p), and both can be expressed as n² + (n+1)² for some integer n (equivalently 2n² + 2n + 1). The only known pair is 12641 ↔ 14621. The search has been extended to 10^24.

## Build Commands

**Prerequisites:** GMP library (`sudo dnf install gmp-devel` on Fedora/RHEL)

```bash
make all          # Build all 6 programs
make clean        # Remove binaries
make distclean    # Remove binaries + all data files
make check-gmp    # Verify GMP installation
make info         # Show system/compiler info
```

Build individual stages: `make stage1` through `make stage5`, plus `make stage3.5`.

**Compiler flags:** `-O3 -march=znver2 -mtune=znver2 -fopenmp -Wall`, linked with `-lgmp`.

## Running

```bash
make test                # Quick test to 10^8 (fast)
make run                 # Standard search to 2×10^19
make extreme             # Search to 10^20
make 10e21               # Power-of-10 targets up through make 10e25
make continue_10e24      # Incremental: extend from 10^23 to 10^24
```

For long runs: `nohup time make 10e23 > run_10e23.log 2>&1 &`

## Pipeline Architecture

The pipeline is 5 stages executed sequentially. Each stage reads from the previous stage's output files. All stages use GMP (GNU Multiple Precision) for arbitrary-precision integers and OpenMP with 16 threads for parallelism.

### Stage 1: Candidate Generation (`generate_candidates_gmp.c`)
- **Input:** command-line arg `<max_prime>`
- **Output:** `p01.dat` through `p16.dat` (16 sharded files, one per thread)
- Computes values of the form 2n² + 2n + 1 up to max_prime
- Applies "gatekeeper" filters on first/last 2 digits to prune non-prime candidates early
- Each of 16 threads handles n ≡ k (mod 16), buffers 10k candidates before writing

### Stage 1 alt: Range Generator (`gen_candidates_range_gmp.c`)
- **Input:** `<start_n> <max_prime>` — for incremental/continuation searches
- Same logic as Stage 1 but generates candidates in a subrange
- Used by `make continue_10e24` and similar incremental targets

### Stage 2: Prime Filter (`filter_primes_gmp.c`)
- **Input:** `p01.dat`–`p16.dat`
- **Output:** `primes01.dat`–`primes16.dat`
- 25-round Miller-Rabin primality test via `mpz_probab_prime_p()`
- 16 threads, one per file

### Stage 3: Emirp Detection (`check_emirp_gmp.c`)
- **Input:** `primes01.dat`–`primes16.dat`
- **Output:** `emirps.dat` (space-separated pairs: original and reversed)
- Reverses each prime's digits, tests if the reversal is also prime
- Skips palindromic primes (handled separately in Stage 3.5)

### Stage 3.5: Palindromic Prime Detection (`check_palindrome_gmp.c`)
- **Input:** `primes01.dat`–`primes16.dat`
- **Output:** `otto_primes.dat`
- Finds palindromic primes that satisfy 2n² + 2n + 1 = p
- Solves the quadratic: n = (-1 + sqrt(2p - 1)) / 2, verifying n is an integer

### Stage 4: Converse Pair Detection (`check_converse_gmp.c`)
- **Input:** `emirps.dat`
- **Output:** `converse.dat` (format: p1 p2 n1 n2)
- For each emirp pair, checks whether both values satisfy n² + (n+1)² form
- Uses perfect-square detection and quadratic solving

## Key Patterns

- **Data flow:** All inter-stage communication is via flat `.dat` files with one number per line (or space-separated pairs for emirps)
- **Parallelism:** 16-way sharding by thread ID (`n % 16`), each thread writes its own file to avoid locking during generation. Stages 3/4 use `#pragma omp critical` for shared output files.
- **GMP usage:** All arithmetic uses `mpz_t` types. Key functions: `mpz_probab_prime_p()` for primality, `mpz_perfect_square_p()` and `mpz_sqrt()` for quadratic solving, `mpz_get_str()`/`mpz_set_str()` for I/O.
- **The quadratic:** The central formula is `p = 2n² + 2n + 1 = n² + (n+1)²`. Solving for n given p: compute `2p - 1`, check if it's a perfect square, then `n = (sqrt(2p-1) - 1) / 2`.

## Documentation

Detailed technical docs are in `docs/cvo-pipe_ref/` (technical overview, command reference, optimization history, performance guide).
