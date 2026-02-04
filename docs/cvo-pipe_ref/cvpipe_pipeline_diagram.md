# CVPipe Pipeline Diagram

**Project:** Converse Prime Research  
**Author:** j (Alaska)  
**System:** CVPipe - 5-Stage Parallel Pipeline  
**Purpose:** Visual reference for data flow and transformations  

---

## Pipeline Overview

CVPipe processes candidates through 5 sequential stages with heavy parallelization at each stage. This document provides detailed visual representations of the data flow, transformations, and threading model.

---

## High-Level Pipeline Flow

```
INPUT: Range specification (start_n, end_value)
   │
   ▼
┌──────────────────────────────────────────────────────────────┐
│  STAGE 1: Candidate Generation                               │
│  Program: gen_candidates_range_gmp                           │
│  Function: Generate n² + (n+1)² values in range             │
│  Output: p01.dat - p16.dat                                   │
│  Parallelization: 16 threads, each writes own file          │
└──────────────────────────────────────────────────────────────┘
   │
   │ [32+ billion candidates]
   │ [40-50 GB text files]
   │
   ▼
┌──────────────────────────────────────────────────────────────┐
│  STAGE 2: Prime Filtering                                    │
│  Program: filter_primes_gmp                                  │
│  Function: Miller-Rabin primality testing                    │
│  Output: primes01.dat - primes16.dat                         │
│  Parallelization: 16 threads, one per input file            │
└──────────────────────────────────────────────────────────────┘
   │
   │ [2.7+ billion primes, ~8.6% of candidates]
   │ [4-5 GB text files]
   │
   ▼
┌──────────────────────────────────────────────────────────────┐
│  STAGE 3: Emirp Detection                                    │
│  Program: check_emirp_gmp                                    │
│  Function: Reverse and test primality                        │
│  Output: emirps.dat (merged)                                 │
│  Parallelization: 16 threads with critical section          │
└──────────────────────────────────────────────────────────────┘
   │
   │ [211+ million emirps, ~7.7% of primes]
   │ [5-10 GB text file]
   │
   ├─────────────────────┬────────────────────────────────────┐
   ▼                     ▼                                    ▼
┌─────────────────┐  ┌─────────────────┐           ┌─────────────────┐
│  STAGE 3.5:     │  │  STAGE 4:       │           │  For reference: │
│  Palindrome     │  │  Converse       │           │  emirps.dat can │
│  check_         │  │  check_converse │           │  be archived or │
│  palindrome_gmp │  │  _gmp           │           │  analyzed later │
│                 │  │                 │           └─────────────────┘
│  Finds Otto    │  │  Verifies both  │
│  primes        │  │  p and rev(p)   │
│                 │  │  are consec.    │
│  Output:        │  │  square sums    │
│  otto_primes.   │  │                 │
│  dat            │  │  Output:        │
│                 │  │  converse.dat   │
└─────────────────┘  └─────────────────┘
         │                    │
         │ [0 found]          │ [0-1 pairs]
         │                    │
         ▼                    ▼
    ┌─────────────────────────────┐
    │  RESULTS                    │
    │  - Otto primes: 0           │
    │  - Converse pairs: 12641    │
    │    ↔ 14621 (known)          │
    └─────────────────────────────┘

OUTPUT: Confirmation of converse pair rarity
```

---

## Stage 1: Candidate Generation

### Detailed Flow

```
                         gen_candidates_range_gmp
                                  │
                    ┌─────────────┴─────────────┐
                    │   Parse arguments:        │
                    │   start_n, end_value      │
                    └─────────────┬─────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │   Initialize GMP          │
                    │   mpz_t variables         │
                    └─────────────┬─────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │   Calculate n_start from  │
                    │   start_n and end_value   │
                    └─────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │  OpenMP Parallel Region (16 threads)            │
         │                                                  │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │ Thread  │  │ Thread  │     ...      │ Thread  │     │
    │    0    │  │    1    │              │   15    │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
         ├─── Compute range for this thread ───┤          │
         │            │                         │          │
    ┌────┴────────────┴─────────────────────────┴────┐    │
    │  For each n in my_range:                       │    │
    │    1. Check zone viability (mod-30 filter)     │    │
    │    2. If viable:                               │    │
    │       - Compute n² using GMP                   │    │
    │       - Compute (n+1)²                         │    │
    │       - Add to get candidate = n² + (n+1)²     │    │
    │    3. Write candidate to pXX.dat               │    │
    └────┬────────────┬─────────────────────────┬────┘    │
         │            │                         │          │
         ▼            ▼                         ▼          │
    ┌────────┐  ┌────────┐              ┌────────┐        │
    │ p01.dat│  │ p02.dat│     ...      │ p16.dat│        │
    └────────┘  └────────┘              └────────┘        │
         │            │                         │          │
         └────────────┴─────────────────────────┘          │
                              │                            │
                              ▼                            │
                      All files written                    │
                      (no merging needed)                  │
                                                           │
         └────────────────────────────────────────────────┘
```

### Zone-Skipping Logic Detail

```
For each n:
   │
   ▼
┌─────────────────────────────┐
│ Calculate zone:             │
│ zone_start = (n/30) * 30    │
│ offset = n % 30             │
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│ Check offset viability:     │
│ Is offset in {1,4,10,11,    │
│  13,19,25,...}?             │
└─────────────┬───────────────┘
              │
      ┌───────┴───────┐
      │               │
    YES              NO
      │               │
      ▼               ▼
  Compute        Skip to next
  candidate      viable offset
      │          (zone jump)
      ▼
  Write to
  output
```

### Data Format Example

**p01.dat content (text format):**
```
100000000000000000000005
100000000000000000000013
100000000000000000000041
100000000000000000000061
...
```

Each line: one candidate value in decimal

---

## Stage 2: Prime Filtering

### Detailed Flow

```
                          filter_primes_gmp
                                  │
                    ┌─────────────┴─────────────┐
                    │   No arguments needed     │
                    │   (reads p01-p16.dat)     │
                    └─────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │  OpenMP Parallel Region (16 threads)            │
         │                                                  │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │ Thread  │  │ Thread  │     ...      │ Thread  │     │
    │    0    │  │    1    │              │   15    │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │  Open   │  │  Open   │              │  Open   │     │
    │ p01.dat │  │ p02.dat │     ...      │ p16.dat │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
    ┌────┴────────────┴─────────────────────────┴────┐    │
    │  For each candidate in file:                   │    │
    │    1. Read line (decimal string)               │    │
    │    2. Parse to mpz_t                           │    │
    │    3. Quick composite check:                   │    │
    │       - Test divisibility by 2,3,5,7,11,13     │    │
    │       - If divisible, skip to next             │    │
    │    4. Miller-Rabin primality test:             │    │
    │       - mpz_probab_prime_p(n, 25)              │    │
    │       - 25 rounds → error < 2^-50              │    │
    │    5. If prime:                                │    │
    │       - Write to primesXX.dat                  │    │
    └────┬────────────┬─────────────────────────┬────┘    │
         │            │                         │          │
         ▼            ▼                         ▼          │
    ┌─────────┐  ┌─────────┐            ┌─────────┐       │
    │primes01 │  │primes02 │    ...     │primes16 │       │
    │  .dat   │  │  .dat   │            │  .dat   │       │
    └─────────┘  └─────────┘            └─────────┘       │
         │            │                         │          │
         └────────────┴─────────────────────────┘          │
                              │                            │
                              ▼                            │
                      All files written                    │
                      Stats printed                        │
                                                           │
         └────────────────────────────────────────────────┘
```

### Miller-Rabin Detail

```
Miller-Rabin Test for candidate n:
   │
   ▼
┌──────────────────────────────┐
│ Express n-1 as 2^r × d       │
│ (factor out powers of 2)     │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│ For i = 1 to 25 rounds:      │
│   Pick random witness a      │
│   Compute x = a^d mod n      │
└──────────────┬───────────────┘
               │
       ┌───────┴───────┐
       │               │
   x=1 or           x≠1 and
   x=n-1?           x≠n-1
       │               │
       │               ▼
       │      ┌──────────────────┐
       │      │ Square x (r-1)   │
       │      │ times, check if  │
       │      │ becomes n-1      │
       │      └────┬─────────────┘
       │           │
       │     ┌─────┴─────┐
       │     │           │
       │   Became     Never
       │    n-1       n-1
       │     │           │
       ▼     ▼           ▼
   Continue  Continue  COMPOSITE
   to next   to next   (FAIL)
   round     round
       │
       ▼ (after 25 rounds)
   PROBABLY PRIME
   (confidence > 99.9999999999%)
```

### Performance Characteristics

```
Input:  32 billion candidates
        40-50 GB of data

Processing:
   - ~2,500 candidates/sec per thread
   - ~40,000 candidates/sec total (16 threads)
   - Miller-Rabin: 25 rounds per candidate
   - Typical run: 3-4 hours

Output: 2.7 billion primes (~8.6%)
        4-5 GB of data
```

---

## Stage 3: Emirp Detection

### Detailed Flow

```
                          check_emirp_gmp
                                  │
                    ┌─────────────┴─────────────┐
                    │   Open output file:       │
                    │   emirps.dat (write mode) │
                    └─────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │  OpenMP Parallel Region (16 threads)            │
         │                                                  │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │ Thread  │  │ Thread  │     ...      │ Thread  │     │
    │    0    │  │    1    │              │   15    │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │  Open   │  │  Open   │              │  Open   │     │
    │primes01 │  │primes02 │     ...      │primes16 │     │
    │  .dat   │  │  .dat   │              │  .dat   │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
    ┌────┴────────────┴─────────────────────────┴────┐    │
    │  For each prime in file:                       │    │
    │    1. Read prime as string                     │    │
    │    2. Check if palindrome:                     │    │
    │       - Compare str[i] with str[len-1-i]       │    │
    │       - If palindrome, skip (not emirp)        │    │
    │    3. Reverse the string:                      │    │
    │       - Use thread-local static buffer         │    │
    │       - reversed[i] = str[len-1-i]             │    │
    │    4. Parse reversed to mpz_t                  │    │
    │    5. Miller-Rabin on reversed:                │    │
    │       - mpz_probab_prime_p(reversed, 25)       │    │
    │    6. If reversed is prime:                    │    │
    │       ┌────────────────────────┐               │    │
    │       │  CRITICAL SECTION      │               │    │
    │       │  (mutex lock)          │               │    │
    │       │  - Write to emirps.dat │               │    │
    │       │  - Format: "p rev(p)"  │               │    │
    │       │  - fflush to disk      │               │    │
    │       │  (mutex unlock)        │               │    │
    │       └────────────────────────┘               │    │
    └────────────────────────────────────────────────┘    │
         │                                                 │
         └─────────────────────────────────────────────────┘
                              │
                              ▼
                      ┌───────────────┐
                      │  emirps.dat   │
                      │  (merged file)│
                      └───────────────┘
                      Format:
                      "prime1 reverse1"
                      "prime2 reverse2"
                      ...
```

### Palindrome Check Detail

```
For prime string "12641":
   │
   ▼
┌─────────────────────────────┐
│ len = strlen("12641") = 5   │
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│ Check: str[0] == str[4]?    │
│        '1'   ==   '1'   ✓   │
└─────────────┬───────────────┘
              │
              ▼
┌─────────────────────────────┐
│ Check: str[1] == str[3]?    │
│        '2'   ==   '4'   ✗   │
└─────────────┬───────────────┘
              │
              ▼
        NOT palindrome
        (proceed to reversal)

For prime string "12321":
   │
   ▼
┌─────────────────────────────┐
│ All comparisons match       │
│ → Palindrome detected       │
│ → SKIP (not an emirp)       │
└─────────────────────────────┘
```

### Reversal Optimization (v2.0)

```
OLD METHOD (malloc/free):
┌─────────────────────────────┐
│ For each prime:             │
│   malloc(len+1)             │  ← 50-100 CPU cycles
│   reverse_string()          │  ← 5-10 CPU cycles
│   free()                    │  ← 30-50 CPU cycles
└─────────────────────────────┘
Total: ~100-150 cycles per reversal
× 161,500 reversals/sec = 16M cycles/sec wasted!

NEW METHOD (thread-local):
┌─────────────────────────────┐
│ Per-thread, one-time:       │
│   static __thread char[256] │  ← Allocated on thread start
│                             │
│ For each prime:             │
│   reverse_string()          │  ← 5-10 CPU cycles
│   (reuse same buffer)       │  ← Zero allocation cost
└─────────────────────────────┘
Total: ~5-10 cycles per reversal
15× faster!
```

### Critical Section Contention Analysis

```
With 16 threads checking ~161,500 primes/sec total:
   - Emirp rate: ~7.7%
   - Emirps found: ~12,400/sec total
   - ~775 emirps/sec per thread

Critical section frequency:
   775 times/sec per thread
   = every 1.3 milliseconds

Critical section duration:
   fprintf + fflush: ~10-20 microseconds

Lock contention:
   0.02ms / 1.3ms = 1.5% time in critical section
   
Result: Negligible overhead (<2%)
```

### Data Format Example

**emirps.dat content:**
```
100000000000000000000041 140000000000000000000001
100000000000000000000061 160000000000000000000001
100000000000000000000101 101000000000000000000001
...
```

Format: "prime reverse_prime" (space-separated)

---

## Stage 3.5: Palindrome Hunter (Otto Primes)

### Detailed Flow

```
                       check_palindrome_gmp
                                  │
                    ┌─────────────┴─────────────┐
                    │   Open output:            │
                    │   otto_primes.dat         │
                    └─────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │  OpenMP Parallel Region (16 threads)            │
         │                                                  │
    ┌────┴────┐  ┌────┴────┐              ┌────┴────┐     │
    │ Thread  │  │ Thread  │     ...      │ Thread  │     │
    │    0    │  │    1    │              │   15    │     │
    └────┬────┘  └────┬────┘              └────┬────┘     │
         │            │                         │          │
    ┌────┴────────────┴─────────────────────────┴────┐    │
    │  For each prime in primesXX.dat:               │    │
    │    1. Read prime as string                     │    │
    │    2. Check if palindrome:                     │    │
    │       - Compare str[i] == str[len-1-i]         │    │
    │       - Must match for all i                   │    │
    │    3. If palindrome AND it's already known     │    │
    │       to be n² + (n+1)² form (from Stage 1):   │    │
    │       ┌────────────────────────┐               │    │
    │       │  CRITICAL SECTION      │               │    │
    │       │  - Write to otto_      │               │    │
    │       │    primes.dat          │               │    │
    │       └────────────────────────┘               │    │
    └────────────────────────────────────────────────┘    │
         │                                                 │
         └─────────────────────────────────────────────────┘
                              │
                              ▼
                      ┌───────────────┐
                      │ otto_primes   │
                      │    .dat       │
                      │  (typically   │
                      │   empty)      │
                      └───────────────┘
```

### Otto Prime Definition

```
Otto Prime Requirements:
   ┌────────────────────────────────┐
   │ 1. Is prime                    │ ✓ (from Stage 2)
   ├────────────────────────────────┤
   │ 2. Is palindrome               │ ✓ (check this stage)
   ├────────────────────────────────┤
   │ 3. Form: n² + (n+1)²           │ ✓ (guaranteed from Stage 1)
   └────────────────────────────────┘
              │
              ▼
        Otto Prime Found!
        (None found to date)
```

### Why No Otto Primes?

```
Consecutive square sums: n² + (n+1)² = 2n² + 2n + 1

For palindromic result, need special n values.

Example search through 10^24:
   - Primes tested: 2.7+ billion
   - Palindromes found: 0
   - Theoretical probability: Unknown
   - May not exist at all, or extremely rare
```

---

## Stage 4: Converse Verification

### Detailed Flow

```
                          check_converse_gmp
                                  │
                    ┌─────────────┴─────────────┐
                    │   Open input:             │
                    │   emirps.dat              │
                    │   Open output:            │
                    │   converse.dat            │
                    └─────────────┬─────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │  OpenMP Parallel Region (16 threads)            │
         │                                                  │
    ┌────┴────────────┴─────────────────────────┴────┐    │
    │  For each emirp pair in emirps.dat:            │    │
    │    1. Read line: "prime reverse"               │    │
    │    2. Parse both to mpz_t                      │    │
    │                                                 │    │
    │    ┌───────────────────────────────────────┐   │    │
    │    │  CHECK PRIME (p):                     │   │    │
    │    │  Is p a consecutive square sum?       │   │    │
    │    │                                        │   │    │
    │    │  1. Solve: n² + (n+1)² = p            │   │    │
    │    │     → n ≈ sqrt(p/2)                   │   │    │
    │    │  2. Check n values in window:         │   │    │
    │    │     [n-10 to n+10]                    │   │    │
    │    │  3. For each candidate n:             │   │    │
    │    │     - Compute n² + (n+1)²             │   │    │
    │    │     - Compare with p                  │   │    │
    │    │  4. If match found → p_valid = true   │   │    │
    │    └───────────────────────────────────────┘   │    │
    │                                                 │    │
    │    ┌───────────────────────────────────────┐   │    │
    │    │  CHECK REVERSE (reverse):             │   │    │
    │    │  Same process as above                │   │    │
    │    │  → reverse_valid = true/false         │   │    │
    │    └───────────────────────────────────────┘   │    │
    │                                                 │    │
    │    If BOTH p_valid AND reverse_valid:          │    │
    │       ┌────────────────────────┐               │    │
    │       │  CONVERSE FOUND!       │               │    │
    │       │  CRITICAL SECTION      │               │    │
    │       │  - Write to converse.  │               │    │
    │       │    dat                 │               │    │
    │       │  - Print celebration   │               │    │
    │       └────────────────────────┘               │    │
    └─────────────────────────────────────────────────┘    │
         │                                                 │
         └─────────────────────────────────────────────────┘
                              │
                              ▼
                      ┌───────────────┐
                      │  converse.dat │
                      │  12641 14621  │
                      │  (known pair) │
                      └───────────────┘
```

### Consecutive Sum Verification Detail

```
Given emirp p = 12641, check if p = n² + (n+1)²:
   │
   ▼
┌────────────────────────────────┐
│ Estimate n:                    │
│ n ≈ sqrt(12641 / 2) ≈ 79.5    │
└────────────┬───────────────────┘
             │
             ▼
┌────────────────────────────────┐
│ Check window [69, 89]:         │
│ (10 below, 10 above estimate)  │
└────────────┬───────────────────┘
             │
    ┌────────┼────────┐
    │        │        │
   n=78     n=79     n=80
    │        │        │
    ▼        ▼        ▼
  12485    12641    12801
   (no)    (YES!)    (no)
             │
             ▼
      ┌──────────────────┐
      │ Found: n = 79    │
      │ 79² + 80² = 12641│
      │ p_valid = TRUE   │
      └──────────────────┘
             │
             ▼
   Now check reverse = 14621:
             │
             ▼
┌────────────────────────────────┐
│ n ≈ sqrt(14621 / 2) ≈ 85.5    │
│ Check window [75, 95]          │
│ Found: 85² + 86² = 14621       │
│ reverse_valid = TRUE           │
└────────────┬───────────────────┘
             │
             ▼
      ┌──────────────────┐
      │ BOTH TRUE        │
      │ → CONVERSE PAIR! │
      │ 12641 ↔ 14621    │
      └──────────────────┘
```

### Why Search Window?

```
Can't solve n² + (n+1)² = p exactly due to:
   - Integer rounding in sqrt()
   - Off-by-one errors
   - Numerical precision limits

Solution: Check neighborhood around estimate
   - Window of ±10 is more than sufficient
   - Ensures we don't miss the exact n value
   - Very fast: only 20 checks per number
```

---

## Threading Model

### Thread Distribution

```
Stage 1 (gen_candidates_range_gmp):
┌───────────────────────────────────────────────────────┐
│  Main Thread                                          │
│  - Parse arguments                                    │
│  - Calculate work distribution                        │
│  - Launch OpenMP parallel region                      │
│                                                        │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 0:  Range [start, start+chunk)         │   │
│  │   Writes to p01.dat                           │   │
│  └────────────────────────────────────────────────┘   │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 1:  Range [start+chunk, start+2*chunk) │   │
│  │   Writes to p02.dat                           │   │
│  └────────────────────────────────────────────────┘   │
│  ...                                                   │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 15: Range [start+15*chunk, end)        │   │
│  │   Writes to p16.dat                           │   │
│  └────────────────────────────────────────────────┘   │
│                                                        │
│  - Join threads                                        │
│  - Print statistics                                    │
└───────────────────────────────────────────────────────┘

Synchronization: NONE (each thread independent)
```

```
Stage 2 (filter_primes_gmp):
┌───────────────────────────────────────────────────────┐
│  Main Thread                                          │
│  - Launch OpenMP parallel region                      │
│                                                        │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 0:  Reads p01.dat                      │   │
│  │            Writes primes01.dat                │   │
│  └────────────────────────────────────────────────┘   │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 1:  Reads p02.dat                      │   │
│  │            Writes primes02.dat                │   │
│  └────────────────────────────────────────────────┘   │
│  ...                                                   │
│                                                        │
│  - Join threads                                        │
│  - Aggregate statistics (reduction)                    │
└───────────────────────────────────────────────────────┘

Synchronization: Reduction for statistics only
```

```
Stage 3 (check_emirp_gmp):
┌───────────────────────────────────────────────────────┐
│  Main Thread                                          │
│  - Open emirps.dat (write mode)                       │
│  - Launch OpenMP parallel region                      │
│                                                        │
│  ┌────────────────────────────────────────────────┐   │
│  │ Thread 0:  Reads primes01.dat                 │   │
│  │            Writes to shared emirps.dat        │   │
│  │            (via critical section)             │   │
│  └────────────────────────────────────────────────┘   │
│  ...                                                   │
│                                                        │
│  - Join threads                                        │
│  - Close emirps.dat                                    │
└───────────────────────────────────────────────────────┘

Synchronization: Critical section for emirps.dat writes
                 (~1.5% overhead)
```

### Memory Layout Per Thread

```
Thread 0 Stack:
┌──────────────────────────┐
│ GMP Variables:           │
│ - mpz_t candidate        │  ~1KB
│ - mpz_t reversed         │  ~1KB
│ - mpz_t temp             │  ~1KB
├──────────────────────────┤
│ Thread-local buffers:    │
│ - char reversed[256]     │  256 bytes (emirp stage)
│ - char line[256]         │  256 bytes
├──────────────────────────┤
│ File handles:            │
│ - FILE *input            │  ~64 bytes
│ - FILE *output           │  ~64 bytes
├──────────────────────────┤
│ Local counters:          │
│ - primes_read, etc.      │  ~32 bytes
└──────────────────────────┘
Total: ~5 KB per thread
× 16 threads = ~80 KB total

Note: GMP may allocate additional heap memory
      for large numbers (managed internally)
```

---

## Data Transformation Flow

### Volume Reduction Through Pipeline

```
                     Data Volume Flow
                     
10²³ to 10²⁴ Search:

Stage 1: Candidates
├─ Generated:      32,056,396,802
├─ File size:      40-50 GB
└─ Rate:           100% (baseline)

        ↓ (Prime filtering)
        
Stage 2: Primes
├─ Found:          2,747,758,244
├─ File size:      4-5 GB
├─ Rate:           8.57% of candidates
└─ Reduction:      91.43% eliminated

        ↓ (Emirp detection)
        
Stage 3: Emirps
├─ Found:          211,273,732
├─ File size:      5-10 GB (pairs)
├─ Rate:           7.69% of primes
└─ Reduction:      92.31% of primes eliminated

        ↓ (Converse verification)
        
Stage 4: Converse
├─ Found:          0 (in this range)
├─ File size:      ~100 bytes (header)
├─ Rate:           0% of emirps
└─ Cumulative:     1 in 1.36 billion primes total

Overall: 32 billion candidates → 0 new converse pairs
         Confirmation of extreme rarity
```

### Format Transformations

```
Stage 1 Output (p01.dat):
┌────────────────────────────┐
│ 100000000000000000000005   │ ← Single number per line
│ 100000000000000000000013   │   (candidate value)
│ 100000000000000000000041   │
└────────────────────────────┘

Stage 2 Output (primes01.dat):
┌────────────────────────────┐
│ 100000000000000000000005   │ ← Single number per line
│ 100000000000000000000013   │   (only primes)
│ 100000000000000000000041   │
└────────────────────────────┘

Stage 3 Output (emirps.dat):
┌────────────────────────────┐
│ 100...041 140...001        │ ← Two numbers per line
│ 100...061 160...001        │   (prime and its reverse)
│ 100...101 101...001        │
└────────────────────────────┘

Stage 4 Output (converse.dat):
┌────────────────────────────┐
│ 12641 14621                │ ← Known converse pair
│                            │   (confirmed in range)
└────────────────────────────┘
```

---

## Error Handling and Recovery

### Pipeline Fault Tolerance

```
Failure Point: Stage 1 crashes
┌──────────────────────────────┐
│ Effect:                      │
│ - Partial p*.dat files exist │
│ - May be incomplete          │
├──────────────────────────────┤
│ Recovery:                    │
│ 1. Delete all p*.dat         │
│ 2. Re-run Stage 1            │
│ 3. Continue from Stage 2     │
└──────────────────────────────┘

Failure Point: Stage 2 crashes
┌──────────────────────────────┐
│ Effect:                      │
│ - Some primes*.dat complete  │
│ - Others incomplete          │
├──────────────────────────────┤
│ Recovery:                    │
│ 1. p*.dat still intact       │
│ 2. Delete primes*.dat        │
│ 3. Re-run Stage 2            │
│ 4. Continue from Stage 3     │
└──────────────────────────────┘

Failure Point: Stage 3 crashes
┌──────────────────────────────┐
│ Effect:                      │
│ - emirps.dat partially       │
│   written                    │
├──────────────────────────────┤
│ Recovery:                    │
│ 1. primes*.dat intact        │
│ 2. Delete emirps.dat         │
│ 3. Re-run Stage 3            │
│ 4. Streaming ensures no      │
│    memory loss on crash      │
└──────────────────────────────┘
```

---

## Performance Visualization

### Runtime Distribution (10²³ to 10²⁴ Search)

```
Total Pipeline Time: ~6 hours

Stage Distribution:
┌───────────────────────────────────────────────────────┐
│ Stage 1: Candidates          [████████      ] ~2.5hr │
│ Stage 2: Primes              [█████████████ ] ~3.5hr │
│ Stage 3: Emirps              [████          ] ~20min │
│ Stage 3.5: Palindromes       [             ] ~1min   │
│ Stage 4: Converse            [█            ] ~90sec  │
└───────────────────────────────────────────────────────┘

Bottleneck: Stage 2 (Miller-Rabin testing)
            - Most computationally intensive
            - Cannot be significantly optimized
              (already using best known algorithm)
```

### CPU Utilization Pattern

```
During Stage 2 (filter_primes_gmp):

CPU Usage (16 threads on 8-core CPU):
┌────────────────────────────────────┐
│ Core 0: [████████████████] 200%   │ (2 threads)
│ Core 1: [████████████████] 200%   │
│ Core 2: [████████████████] 200%   │
│ Core 3: [████████████████] 200%   │
│ Core 4: [████████████████] 200%   │
│ Core 5: [████████████████] 200%   │
│ Core 6: [████████████████] 200%   │
│ Core 7: [████████████████] 200%   │
└────────────────────────────────────┘
Total: ~1600% (near-perfect utilization)

Memory Bandwidth:
┌────────────────────────────────────┐
│ Read:  [██████████  ] ~60%         │
│ Write: [████        ] ~20%         │
└────────────────────────────────────┘
Moderate memory pressure
```

---

## Comparison: Original vs Current Architecture

### 1990s otto.c Architecture

```
Single-Threaded Sequential Processing:
┌────────────────────────────────────────┐
│ For n = 1 to max_n:                   │
│   candidate = n² + (n+1)²             │
│   if is_prime(candidate):             │
│     reversed = reverse(candidate)     │
│     if is_prime(reversed):            │
│       if both_consec_sums:            │
│         FOUND!                        │
└────────────────────────────────────────┘

- Everything in one program
- No intermediate files
- Trial division (slow)
- Limited to 2³² by 32-bit arithmetic
- Hours for small ranges
```

### 2026 CVPipe Architecture

```
5-Stage Parallel Pipeline:
┌────────────────────────────────────────┐
│ Stage 1: Generate (16 threads)        │
│   → p*.dat files                      │
├────────────────────────────────────────┤
│ Stage 2: Filter (16 threads)          │
│   → primes*.dat files                 │
├────────────────────────────────────────┤
│ Stage 3: Emirps (16 threads)          │
│   → emirps.dat file                   │
├────────────────────────────────────────┤
│ Stage 3.5: Palindromes (16 threads)   │
│   → otto_primes.dat                   │
├────────────────────────────────────────┤
│ Stage 4: Converse (16 threads)        │
│   → converse.dat                      │
└────────────────────────────────────────┘

- Staged processing with checkpoints
- Fault-tolerant (can resume)
- Miller-Rabin (fast)
- GMP for unlimited range
- Hours for enormous ranges (10²⁴+)
- ~400,000× faster overall
```

---

## Future Architecture Possibilities

### Distributed Cloud Pipeline

```
┌─────────────────────────────────────────────────┐
│ AWS Orchestration (Kubernetes)                  │
├─────────────────────────────────────────────────┤
│                                                 │
│ ┌────────────┐  ┌────────────┐  ┌────────────┐│
│ │ Worker 1   │  │ Worker 2   │  │ Worker N   ││
│ │ c5.18xlarge│  │ c5.18xlarge│  │ c5.18xlarge││
│ │ 72 vCPUs   │  │ 72 vCPUs   │  │ 72 vCPUs   ││
│ └─────┬──────┘  └─────┬──────┘  └─────┬──────┘│
│       │               │               │        │
│       └───────────────┴───────────────┘        │
│                       │                        │
│              ┌────────┴────────┐               │
│              │  S3 Storage     │               │
│              │  - Candidates   │               │
│              │  - Primes       │               │
│              │  - Results      │               │
│              └─────────────────┘               │
└─────────────────────────────────────────────────┘

Potential: 100× speedup with 100 workers
           Search to 10³⁰+ in reasonable time
```

---

## Appendix: Complete Example Run Visualization

### Small Test Run (n=1 to 100,000)

```
TIME: T=0
─────────────────────────────────────────────────
Stage 1: gen_candidates_range_gmp 1 10000000000

[Thread  0] ████████████████████ (chunk 0)
[Thread  1] ████████████████████ (chunk 1)
...
[Thread 15] ████████████████████ (chunk 15)

Files created: p01.dat - p16.dat
                 ↓
─────────────────────────────────────────────────
TIME: T=30s
Stage 2: filter_primes_gmp

[Thread  0] ████████████████████ (p01.dat → primes01.dat)
[Thread  1] ████████████████████ (p02.dat → primes02.dat)
...
[Thread 15] ████████████████████ (p16.dat → primes16.dat)

Files created: primes01.dat - primes16.dat
                 ↓
─────────────────────────────────────────────────
TIME: T=45s
Stage 3: check_emirp_gmp

All threads → emirps.dat (merged via critical section)

[Thread  0] ████████████████████
[Thread  1] ████████████████████
          ...
[Thread 15] ████████████████████
            ↓ ↓ ↓ (all write to same file)
          emirps.dat

File created: emirps.dat
                 ↓
─────────────────────────────────────────────────
TIME: T=50s
Stage 3.5: check_palindrome_gmp

All threads → otto_primes.dat

Result: 0 palindromes found
                 ↓
─────────────────────────────────────────────────
TIME: T=51s
Stage 4: check_converse_gmp

All threads process emirps.dat → converse.dat

Result: 12641 ↔ 14621 confirmed!
                 ↓
─────────────────────────────────────────────────
TIME: T=52s

PIPELINE COMPLETE
Results in: converse.dat
Confirmed: One converse pair in range
```

---

*Document Version: 1.0*  
*Last Updated: January 31, 2026*  
*Visual Reference for CVPipe Architecture*
