# CVPipe — Converse Prime Pipeline

CVPipe searches for **converse primes**, an extremely rare class of prime pairs. A converse prime pair (p, q) requires:

- Both p and q are prime
- q = reverse(p) (digit reversal)
- Both can be expressed as n² + (n+1)² for some integer n

The only known pair is **12641 ↔ 14621** (n=79, n=85). The search has been extended to 10^24 with no additional pairs found.

## The Math

The central formula is `p = 2n² + 2n + 1 = n² + (n+1)²`. Given a candidate prime p, solving for n: compute `2p - 1`, check if it's a perfect square, then `n = (sqrt(2p-1) - 1) / 2`.

## Quick Start

**Prerequisites:** GCC, GMP library, OpenMP

```bash
# Fedora/RHEL
sudo dnf install gmp-devel

# Ubuntu/Debian
sudo apt install libgmp-dev
```

**Build and test:**

```bash
make cvpipe
./cvpipe 100000000          # Quick test to 10^8 (~instant)
```

Expected output: 1 converse pair (12641 ↔ 14621), 3 otto primes.

## Usage

```bash
./cvpipe <max_prime>                  # Full search from n=0
./cvpipe <start_n> <max_prime>        # Continue from start_n
```

**Makefile targets:**

```bash
make cvpipe-test       # Quick test to 10^8
make cvpipe-10e20      # Search to 10^20
make cvpipe-10e25      # Search to 10^25
```

**Long runs:**

```bash
nohup time ./cvpipe 10000000000000000000000000 > run_10e25.log 2>&1 &
```

## How It Works

CVPipe merges 5 pipeline stages into a single in-memory pass — no intermediate files:

1. **Generate** candidate p = 2n² + 2n + 1
2. **Gatekeeper** filter on first/last 2 digits (proven digit constraints)
3. **Zone-skip** iterates only n-ranges producing valid first-2-digit patterns, eliminating ~89% of candidates
4. **Consec-sq pre-filter** on reversed digits (skip ~96% of Miller-Rabin tests)
5. **Miller-Rabin** primality (25 rounds) on p and reverse(p)
6. **Converse check** — verify both p and reverse(p) satisfy n² + (n+1)²

### Zone-Skip Optimization

At startup, CVPipe computes valid search zones from 6 proven first-2-digit patterns: {10, 12, 14, 16, 18, 31}. Threads iterate zone-by-zone instead of linearly, giving ~9x fewer candidates at scale.

### Multi-Machine Runs

Split the search by giving each machine a different prime sub-range:

```bash
# Machine A (zones 1-2, patterns 10+12)
./cvpipe 707106781186 1299999999999999999999999

# Machine B (zones 3-6, patterns 14+16+18+31)
./cvpipe 836660026533 10000000000000000000000000
```

## Output Files

- `converse.dat` — Converse pairs found (format: p1 p2 n1 n2)
- `otto_primes.dat` — Palindromic primes that are sums of consecutive squares

## Technical Details

- **Language:** C with GMP (GNU Multiple Precision Arithmetic)
- **Parallelism:** OpenMP, 16 threads
- **Compiler flags:** `-O3 -march=znver2 -mtune=znver2 -fopenmp -Wall -lgmp`
- **Primality:** 25-round Miller-Rabin via `mpz_probab_prime_p()`

## Legacy Pipeline

The original 5-stage pipeline (with intermediate `.dat` files) is also included:

| Stage | Program | Purpose |
|-------|---------|---------|
| 1 | `generate_candidates_gmp` | Generate candidates |
| 1-alt | `gen_candidates_range_gmp` | Range/continuation mode |
| 2 | `filter_primes_gmp` | Miller-Rabin filter |
| 3 | `check_emirp_gmp` | Emirp detection |
| 3.5 | `check_palindrome_gmp` | Palindromic prime detection |
| 4 | `check_converse_gmp` | Converse pair detection |

Build all stages with `make all`.

## License

Research project — converse prime hunting.
