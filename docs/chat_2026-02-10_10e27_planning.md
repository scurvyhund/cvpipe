# CVPipe Chat Log — 2026-02-10
## Planning the 10^27 Run

---

### 1. Analysis of run_10e26.log

**Last zone checked (Zone 151):**
- Pattern: 10, Digits: 27
- n_min = n_max = 7,071,067,811,864 (single n-value at upper boundary)

**Run summary:**
- Max prime: 10^26, Max N: 7,071,067,811,864
- 151 zones, 16 threads, 25 MR rounds
- Zone n-values: 781,361,473,295 (88.9% eliminated by zone sieve, 9.0x speedup)
- Pipeline: 781B candidates -> 469B gatekeeper passed -> 19 palindromes -> 26 consec-sq -> 54 MR tests -> 12 primes -> 2 emirps -> 1 con-verse pair -> 3 otto primes
- Wall time: 34,596s (9h 36m), 788% CPU utilization
- Result: Exactly one con-verse pair: 12641 <==> 14621

### 2. Zone Sieve Correctness Discussion

The gap from ~3.2x10^25 to ~10^26 (leading digits 32-99) is correctly excluded because:

- Formula 2n^2+2n+1 can only end in digits: 01, 21, 41, 61, 81 (last digit 1) or 13 (last digit 3)
- For primes, last digit must be 1 or 3 (not 5)
- For con-verse pairs, the reverse's first two digits = original's last two digits reversed
- This gives exactly 6 valid leading-digit patterns: {10, 12, 14, 16, 18, 31}
- The set is self-consistent (closed under digit reversal of last-two/first-two)
- All other leading digits are provably unproductive

### 3. Runtime Estimation (10^25 vs 10^26 data)

**Raw data:**

| Metric           | 10^25 run        | 10^26 run         | Ratio |
|------------------|------------------|--------------------|-------|
| Max N            | 2,236,067,977,499| 7,071,067,811,864  | 3.162 |
| Zone n-vals      | 247,088,193,248  | 781,361,473,295    | 3.162 |
| Wall time (s)    | 10,786           | 34,596             | 3.208 |

**Scaling model:**
- Zone n-vals grow by sqrt(10) = 3.162x per decade
- Cost per candidate grows ~2% per decade (larger numbers slightly more expensive)
- Incremental time grows by 3.162 x 1.02 = 3.225x per decade

**Incremental run estimates (each from previous checkpoint, 16 threads):**

| Run              | Zone n-vals | ns/cand | Wall time    |
|------------------|------------|---------|--------------|
| 10^25 -> 10^26   | 534B       | 44.6    | 23,810s (6.6h) |
| 10^26 -> 10^27   | 1,689B     | 45.5    | 76,787s (21.3h) |
| 10^27 -> 10^28   | 5,340B     | 46.4    | 247,638s (2.9d) |
| 10^28 -> 10^29   | 16,886B    | 47.3    | 798,632s (9.2d) |
| 10^29 -> 10^30   | 53,383B    | 48.3    | 2,575,588s (29.8d) |

**Cumulative from 10^26 to 10^30: ~42.8 days**

### 4. Code Audit of cvpipe.c for 10^27+

**Verdict: CLEAN for 10^27 — no blockers.**

All n-values (~2.2x10^13) fit in uint64_t. Zone count (~157) fits MAX_ZONES=256. String buffers (256 bytes) ample for 28-digit numbers.

**Future limits:**

| Limit                    | Breaks at | Root cause                    |
|--------------------------|-----------|-------------------------------|
| MAX_ZONES = 256          | ~10^43    | 6 zones per digit count       |
| uint64_t for n           | ~10^38    | n = sqrt(P/2) exceeds 2^64   |
| mpz_get_ui() truncation  | ~10^38    | Silent wrong results          |
| cand_str[256] buffer     | ~10^255   | Theoretical only              |

### 5. 10^27 Run Parameters

```
start_n   = 7,071,067,811,865
max_prime = 1,000,000,000,000,000,000,000,000,000  (10^27)
max_n     = 22,360,679,774,997
```

**Launch command:**
```bash
nohup time ./cvpipe 7071067811865 1000000000000000000000000000 > run_10e27.log 2>&1 &
```

**Important:** Second argument is max_prime (not max_n). The program internally converts max_prime to max_n via n = floor(sqrt(P/2)).

**Estimated wall time: ~21 hours (16 threads)**

### 6. Decision

10^28 (~2.9 days) considered the practical local hardware limit before needing cloud resources. To be reassessed after 10^27 results.
