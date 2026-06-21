# CVPipe — Converse Prime Pipeline

CVPipe searches for **converse primes** — an extremely rare class of prime
pair on the curve **p = 2n² + 2n + 1 = n² + (n+1)²** (the sum of two
consecutive squares). In standard number-theory terminology these are
**bi-quadratic emirps**: both p and its digit-reversal q = rev(p) are prime,
both lie on the same quadratic curve, and p ≠ q. The "bi-quadratic" name
comes from *both* primes satisfying the same quadratic form 2n²+2n+1 —
two numbers, one curve.

The **only known pair** is **12641 ↔ 14621** (n = 79 and n = 85).
No additional pair has been found through 10²⁴.

CVPipe also collects **otto primes** — palindromic primes on the curve
(p = rev(p); also called prime palindromes on 2n²+2n+1).
The only otto primes found: **5, 181, 313, 3187813**.

---

## The curve

Every value of p(n) = 2n² + 2n + 1 is a **sum of two consecutive squares**,
n² + (n+1)², and is therefore ≡ 1 (mod 4) — the tightest special case of
the primes in Fermat's two-square theorem. This structure pins every value
to one of just **six two-digit endings** `{01, 13, 21, 41, 61, 81}`, proven
by construction, which drives the zone-skip filter below.

Given a candidate prime p, the inverse formula recovers n:

    n = (sqrt(2p − 1) − 1) / 2

If that n is a non-negative integer, p is on the curve.

---

## Results

| object | result | verified through |
|---|---|---|
| Converse prime pair (bi-quadratic emirp) | **12641 ↔ 14621** only | 10²⁴ (CVPipe) |
| Otto prime (palindromic prime on curve) | **5, 181, 313, 3187813** | 10²⁴ (CVPipe) |

The deeper brute-force verification — emirps through d = 26 digits,
palindromes through d = 27 — was done by the successor tools in
[bi-quad](https://github.com/scurvyhund/bi-quad), which grew out of CVPipe.

---

## Quick start

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
./cvpipe 100000000    # quick test to 10^8 (~instant)
```

Expected output: 1 converse pair (12641 ↔ 14621), 3 otto primes.

---

## Usage

```bash
./cvpipe <max_prime>                 # full search from n=0
./cvpipe <start_n> <max_prime>       # continue from start_n
```

**Makefile targets:**

```bash
make cvpipe-test      # quick test to 10^8
make cvpipe-10e20     # search to 10^20
make cvpipe-10e25     # search to 10^25
```

**Long runs:**

```bash
nohup time ./cvpipe 10000000000000000000000000 \
  > run_10e25.log 2>&1 &
```

---

## How it works

CVPipe merges 5 pipeline stages into a single in-memory pass — no
intermediate files:

1. **Generate** candidate p = 2n² + 2n + 1
2. **Gatekeeper** filter on first/last 2 digits (proven digit constraints)
3. **Zone-skip** iterates only n-ranges that produce valid first-2-digit
   patterns, eliminating ~89% of candidates before any arithmetic
4. **Consec-sq pre-filter** on reversed digits (skips ~96% of Miller-Rabin)
5. **Miller-Rabin** primality (25 rounds) on p and rev(p)
6. **Converse check** — verify both p and rev(p) satisfy n² + (n+1)²

### Zone-skip: the key optimization

At startup CVPipe computes valid search zones from the 6 proven first-2-digit
patterns `{10, 12, 14, 16, 18, 31}`. Threads iterate zone-by-zone rather
than linearly — ~9× fewer candidates at scale. This breakthrough
(62.8M× speedup over the original pipeline) was first achieved in CVPipe
before the project was restructured into bi-quad.

### Multi-machine runs

Split the search by prime sub-range:

```bash
# Machine A (zones 1–2, patterns 10+12)
./cvpipe 707106781186 1299999999999999999999999

# Machine B (zones 3–6, patterns 14+16+18+31)
./cvpipe 836660026533 10000000000000000000000000
```

---

## Output files

- `converse.dat` — converse pairs found (format: p1 p2 n1 n2)
- `otto_primes.dat` — otto primes (palindromic primes on the curve)

---

## Legacy pipeline

The original 5-stage pipeline (with intermediate `.dat` files) is
also included:

| Stage | Program | Purpose |
|---|---|---|
| 1 | `generate_candidates_gmp` | Generate candidates |
| 1-alt | `gen_candidates_range_gmp` | Range/continuation mode |
| 2 | `filter_primes_gmp` | Miller-Rabin filter |
| 3 | `check_emirp_gmp` | Emirp detection |
| 3.5 | `check_palindrome_gmp` | Palindromic prime detection |
| 4 | `check_converse_gmp` | Converse pair detection |

Build all stages with `make all`.

---

## Technical details

- **Language:** C with GMP (GNU Multiple Precision Arithmetic Library)
- **Parallelism:** OpenMP, 16 threads
- **Compiler flags:** `-O3 -march=znver2 -mtune=znver2 -fopenmp -Wall -lgmp`
- **Primality:** 25-round Miller-Rabin via `mpz_probab_prime_p()`

---

## Origins

The hunt began after Simon Singh's *Fermat's Enigma* (1997) — a book about
Fermat's *Last* Theorem that pointed toward his *two-square* theorem. The
first converse pair, **12641 ↔ 14621**, was found by an earlier brute-force
tool. CVPipe was built to search faster and deeper. The zone-skip
optimization — the 62.8M× breakthrough — originated here.

The successor project [bi-quad](https://github.com/scurvyhund/bi-quad)
extended the search with GMP-exact brute force (emirps through 26 digits,
prime palindromes through 27 digits) and a modular obstruction sieve.

---

## License

Research project — converse prime / bi-quadratic emirp hunting.
