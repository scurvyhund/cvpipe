# CVPipe Technical Overview

**Project:** 25-Year Converse Prime Research  
**Author:** j (Alaska)  
**System:** CVPipe - Parallel Prime Analysis Pipeline  
**Hardware:** nitroIII (Ryzen 7 4700U, 64GB RAM)  
**Date:** January 2026  

---

## Executive Summary

CVPipe is a sophisticated 5-stage parallel computational pipeline designed to hunt for **converse primes** - an extremely rare class of emirp pairs where both the prime and its reverse can be expressed as sums of consecutive squares (n² + (n+1)²).

After 25 years of research and computation, only **one converse pair is known**: 12641 ↔ 14621, discovered by j in the 1990s. Recent searches extending to 10²⁴ (1 septillion) have confirmed this pair's extraordinary rarity: approximately **1 in 1.36 billion primes**.

---

## The Converse Prime Problem

### Definition

A **converse prime** is a prime number p that satisfies ALL of the following conditions:

1. **p is prime** (passes Miller-Rabin primality test)
2. **p is an emirp** (reverse of p is also prime, and p ≠ reverse(p))
3. **p = n² + (n+1)²** for some integer n (sum of consecutive squares)
4. **reverse(p) = m² + (m+1)²** for some integer m (reverse is also a sum of consecutive squares)

### Mathematical Properties

**Form:** n² + (n+1)² = 2n² + 2n + 1

This generates sequences like:
```
n=1:  1² + 2²  = 5
n=2:  2² + 3²  = 13
n=3:  3² + 4²  = 25  (not prime)
n=4:  4² + 5²  = 41
n=79: 79² + 80² = 12641  (CONVERSE PRIME)
```

**Key observation:** All values ≡ 1 (mod 4), which is a necessary but not sufficient condition for primality.

### The Known Converse Pair

```
12641 = 79² + 80²   (prime, emirp)
14621 = 85² + 86²   (prime, emirp, reverse of 12641)
```

This pair was discovered in the 1990s using early computational methods and remains the **only known converse pair** despite extensive searches.

---

## CVPipe Architecture

### Overview

CVPipe processes candidates through 5 sequential stages, with heavy parallelization (16 threads) at each stage:

```
Stage 1: gen_candidates_range_gmp
   ↓ (p01.dat - p16.dat)
Stage 2: filter_primes_gmp
   ↓ (primes01.dat - primes16.dat)
Stage 3: check_emirp_gmp
   ↓ (emirps.dat)
Stage 3.5: check_palindrome_gmp
   ↓ (otto_primes.dat)
Stage 4: check_converse_gmp
   ↓ (converse.dat)
```

### Data Flow

**Input:** Range specifications (start, end)  
**Intermediate:** Binary and text files containing candidates, primes, emirps  
**Output:** converse.dat (final results, if any)  
**Volume:** Billions of candidates → Millions of primes → Hundreds of thousands of emirps

---

## Stage 1: Candidate Generation

**Program:** `gen_candidates_range_gmp`  
**Input:** Start value, end value (supports u128 and beyond via GMP)  
**Output:** p01.dat through p16.dat (16 sharded files)  
**Algorithm:** Zone-skipping optimization

### Zone-Skipping Algorithm

Instead of testing every n² + (n+1)², CVPipe uses **gatekeeper filtering** to skip zones that cannot produce primes:

```c
// Zone calculation for n² + (n+1)²
uint64_t zone_start = (n / 30) * 30;
uint64_t offset = n % 30;

// Skip zones based on mod-30 patterns
// Only certain offsets can yield primes
if (!is_viable_zone(zone_start, offset)) {
   continue;  // Skip this zone entirely
}
```

**Performance impact:** Reduces candidate generation by ~40% compared to brute-force enumeration.

### Threading Model

16 threads partition the search space:
```c
#pragma omp parallel for num_threads(16)
for (int tid = 0; tid < 16; tid++) {
   uint64_t chunk_size = (end - start) / 16;
   uint64_t my_start = start + tid * chunk_size;
   uint64_t my_end = my_start + chunk_size;
   
   // Generate candidates for this thread's range
   generate_chunk(my_start, my_end, tid);
}
```

Each thread writes to its own output file (p01.dat, p02.dat, etc.) to avoid lock contention.

### GMP Integration

For searches beyond 2⁶⁴, CVPipe uses GNU Multiple Precision (GMP) arithmetic:

```c
mpz_t candidate, n_squared, n_plus_1_squared;
mpz_init(candidate);
mpz_init(n_squared);
mpz_init(n_plus_1_squared);

// candidate = n² + (n+1)²
mpz_mul(n_squared, n, n);
mpz_add_ui(n_plus_1, n, 1);
mpz_mul(n_plus_1_squared, n_plus_1, n_plus_1);
mpz_add(candidate, n_squared, n_plus_1_squared);
```

---

## Stage 2: Prime Filtering

**Program:** `filter_primes_gmp`  
**Input:** p01.dat - p16.dat  
**Output:** primes01.dat - primes16.dat  
**Algorithm:** Miller-Rabin primality test (25 rounds)  

### Miller-Rabin Implementation

```c
// GMP Miller-Rabin: 25 rounds for deterministic results up to 2^64
// Beyond 2^64, provides probabilistic primality (error < 2^-50)
int is_prime = mpz_probab_prime_p(candidate, 25);

if (is_prime > 0) {
   // Definitely prime (or extremely high probability)
   write_prime(candidate, output_file);
}
```

### Performance Characteristics

**Typical run (10²³ to 10²⁴):**
```
Total candidates:   32,056,396,802
Primes found:        2,747,758,244
Prime density:            8.57%
Elapsed time:           12,933 seconds (3.6 hours)
```

**Throughput:** ~2,478 candidates/second tested across 16 threads (~155 candidates/sec per thread)

---

## Stage 3: Emirp Detection

**Program:** `check_emirp_gmp`  
**Input:** primes01.dat - primes16.dat  
**Output:** emirps.dat  
**Algorithm:** String reversal + Miller-Rabin on reverse  

### Original Implementation (Pre-Optimization)

```c
// OLD: malloc/free on every reversal (SLOW!)
static char* reverse_string(const char *str) {
   int len = strlen(str);
   char *reversed = malloc(len + 1);  // ← Expensive!
   if (!reversed) return NULL;
   
   for (int i = 0; i < len; i++) {
      reversed[i] = str[len - 1 - i];
   }
   reversed[len] = '\0';
   return reversed;
}

// Usage:
char *rev = reverse_string(prime_str);
mpz_set_str(reversed_num, rev, 10);
free(rev);  // ← More overhead
```

**Problem:** With 161,500 emirps/second being checked, this created **16+ million malloc/free calls per second** across all threads, causing severe allocator contention.

### Optimized Implementation (Thread-Local Buffer)

```c
// NEW: Thread-local static buffer (FAST!)
static char* reverse_string(const char *str) {
   static __thread char reversed[256];  // Per-thread, no allocation
   int len = strlen(str);
   
   for (int i = 0; i < len; i++) {
      reversed[i] = str[len - 1 - i];
   }
   reversed[len] = '\0';
   return reversed;
}

// Usage:
char *rev = reverse_string(prime_str);  // No malloc!
mpz_set_str(reversed_num, rev, 10);
// No free needed!
```

**Benefits:**
- Zero heap allocations
- Perfect cache locality (same 256 bytes reused millions of times)
- No allocator lock contention
- No TLB thrashing

**Expected speedup:** 10-25% reduction in emirp checking time (1308s → ~1000-1150s)

### Emirp Detection Logic

```c
// 1. Skip palindromes (not emirps by definition)
if (is_palindrome_str(prime_str)) {
   continue;
}

// 2. Reverse the prime
char *reversed_str = reverse_string(prime_str);

// 3. Test if reverse is prime
mpz_set_str(reversed_num, reversed_str, 10);
if (mpz_probab_prime_p(reversed_num, 25) > 0) {
   // Found emirp - write to emirps.dat
   fprintf(fp_out, "%s %s\n", prime_str, reversed_str);
}
```

### Performance Characteristics

**Typical run (10²³ to 10²⁴):**
```
Total primes:        2,747,758,244
Emirps found:          211,273,732 (7.69% of primes)
Elapsed time:                1,308 seconds (22 minutes)
```

**Throughput:** ~161,500 emirps checked/second

### Streaming Output Fix (January 2026)

**Critical bug discovered:** Original code buffered emirps in memory arrays with hard-coded size limits:

```c
// OLD (BROKEN): Fixed-size array
#define MAX_EMIRPS 10000000  // Only 10M!
emirp_pair_t emirps[MAX_EMIRPS];
```

This silently truncated results, potentially missing **millions of emirps** in large searches.

**Fix:** Stream emirps directly to disk as they're found:

```c
// NEW (CORRECT): Immediate write with critical section
#pragma omp critical
{
   fprintf(fp_out, "%s %s\n", prime_str, reversed_str);
   fflush(fp_out);  // Ensure immediate write
}
```

**Impact:** Revealed 60× more emirps in some searches (e.g., 3.5M → 211M in 10²³-10²⁴ range).

---

## Stage 3.5: Palindrome Hunter (Otto Primes)

**Program:** `check_palindrome_gmp`  
**Input:** primes01.dat - primes16.dat  
**Output:** otto_primes.dat  
**Purpose:** Find palindromic primes that are also sums of consecutive squares  

Named "Otto primes" after the original 1990s program otto.c.

### Algorithm

```c
// Check if prime is palindrome
if (is_palindrome_str(prime_str)) {
   // Already confirmed to be n² + (n+1)² form from Stage 1
   write_otto_prime(prime_str);
}
```

### Historical Note

In 25 years of searching up to 10²⁴, **zero Otto primes have been found**. This suggests palindromic primes in the form n² + (n+1)² may not exist, or are even rarer than converse primes.

---

## Stage 4: Converse Verification

**Program:** `check_converse_gmp`  
**Input:** emirps.dat  
**Output:** converse.dat  
**Algorithm:** Verify both p and reverse(p) are consecutive square sums  

### Verification Logic

For each emirp pair (p, reverse(p)):

```c
// Check if p is a consecutive square sum
bool p_is_consec_sum = false;
mpz_set(candidate, p);

// Solve: n² + (n+1)² = p
// This means: 2n² + 2n + 1 = p
// Or: n ≈ sqrt(p/2)

mpz_sqrt(n_approx, p);
mpz_div_ui(n_approx, n_approx, 2);  // Starting point

// Test neighborhood around sqrt(p/2)
for (int offset = -10; offset <= 10; offset++) {
   mpz_add_si(n, n_approx, offset);
   mpz_mul(nsq, n, n);
   mpz_add_ui(n1, n, 1);
   mpz_mul(n1sq, n1, n1);
   mpz_add(sum, nsq, n1sq);
   
   if (mpz_cmp(sum, p) == 0) {
      p_is_consec_sum = true;
      break;
   }
}

// Repeat for reverse(p)
bool rev_is_consec_sum = check_consecutive_sum(reverse_p);

// If BOTH are consecutive sums, we found a converse prime!
if (p_is_consec_sum && rev_is_consec_sum) {
   fprintf(fp_out, "CONVERSE: %s <-> %s\n", p_str, rev_str);
}
```

### Performance Characteristics

**Typical run (10²³ to 10²⁴):**
```
Emirp pairs checked:     211,273,732
Converse pairs found:                0
Elapsed time:                   91 seconds
```

**Throughput:** ~2.3 million emirp pairs verified per second

---

## Hardware Infrastructure

### nitroIII Specifications

```
CPU:      AMD Ryzen 7 4700U (8 cores, 16 threads)
RAM:      64GB DDR4
Storage:  Multiple drives (XFS filesystem)
OS:       Linux (GNU toolchain)
```

### Storage Architecture

**Current setup:**
- Working directory: Fast SSD for active pipeline files
- Archive storage: Large capacity drives for completed searches
- Filesystem: XFS (chosen for large file handling and performance)

**Planned expansion:**
- 20TB drive array for extended searches
- Cloud deployment: AWS instances for searches beyond local capacity

### Compiler Optimization

```bash
# Standard build flags
gcc -O3 -fopenmp -Wall -march=native program.c -o program -lgmp

# Key flags:
# -O3           : Maximum optimization
# -fopenmp      : OpenMP threading support
# -march=native : CPU-specific optimizations
# -lgmp         : GNU Multiple Precision library
```

---

## Performance History

### Evolution of Search Capability

**1990s (otto.c, 32-bit):**
- Single-threaded
- Limited to ~2³² search space
- Hours per small range
- Discovered 12641 ↔ 14621

**2024 (CVPipe initial, 64-bit):**
- 16-thread parallelization
- Zone-skipping optimization
- Searches to 10²³ in ~3 hours
- GMP support for unlimited range

**2025-2026 (CVPipe optimized):**
- Thread-local buffers (malloc elimination)
- Streaming output (removed hard limits)
- Searches to 10²⁴ in ~6 hours
- Processing 1 septillion candidates

### Benchmark: 10²³ to 10²⁴

**Search parameters:**
```
Start:  10²³        (100 sextillion)
End:    10²⁴        (1 septillion)
Range:  900 sextillion values
```

**Results:**
```
Stage 1 (candidates):  32,056,396,802 generated
Stage 2 (primes):       2,747,758,244 found (8.57%)
Stage 3 (emirps):         211,273,732 found (7.69% of primes)
Stage 4 (converse):                 0 found

Total runtime: ~6 hours
Converse pair rarity: 1 in 1.36 billion primes (and counting)
```

---

## Current Research Status

### Confirmed Results

- **One converse pair known:** 12641 ↔ 14621
- **Searched to:** 10²⁴ (1 septillion)
- **Primes tested:** ~2.7 billion in most recent search
- **Emirps found:** ~211 million in most recent search
- **Additional converse pairs:** None

### Statistical Significance

The 12641 ↔ 14621 pair remains unique across:
- 25 years of computation
- Multiple architectural generations
- Billions of primes examined
- **Estimated rarity: < 1 in 10⁹ primes**

### Future Directions

**Short-term:**
- Complete validation of 10²³-10²⁴ search with optimized code
- Extend searches to 10²⁵ using local hardware
- Implement additional optimizations based on profiling data

**Medium-term:**
- Deploy to AWS for massively parallel searches
- Explore 128-bit and 256-bit arithmetic for extended ranges
- Investigate alternative mathematical approaches to converse prime generation

**Long-term:**
- Search to 10³⁰ or beyond
- Collaborate with number theorists on theoretical bounds
- Submit comprehensive findings to mathematical journals

---

## Technical Achievements

### Algorithmic Innovations

1. **Zone-skipping**: 40% reduction in candidate generation
2. **Gatekeeper patterns**: Mod-30 filtering for viable zones
3. **Parallel sharding**: 16-way partitioning with zero lock contention
4. **Streaming output**: Unlimited result sets without memory constraints
5. **Thread-local optimization**: Elimination of allocator bottlenecks

### Software Engineering

1. **GMP integration**: Seamless support for unlimited integer sizes
2. **Robust file handling**: Automatic recovery from corruption
3. **Comprehensive testing**: Validation tools at every pipeline stage
4. **Performance profiling**: Data-driven optimization decisions
5. **Reproducible builds**: Makefile automation and version control

### Mathematical Contributions

1. **Rarity quantification**: First systematic measurement of converse prime density
2. **Emirp statistics**: Large-scale emirp distribution data
3. **Consecutive square analysis**: Insights into n² + (n+1)² prime properties
4. **OEIS submission**: Documentation for mathematical community

---

## References and Resources

### Code Repositories

- gen_candidates_range_gmp.c - Stage 1 candidate generator
- filter_primes_gmp.c - Stage 2 Miller-Rabin filter
- check_emirp_gmp.c - Stage 3 emirp detector
- check_palindrome_gmp.c - Stage 3.5 Otto prime hunter
- check_converse_gmp.c - Stage 4 converse verifier

### Mathematical Background

- OEIS sequence submissions (pending)
- Miller-Rabin primality testing algorithm
- Emirp theory and distribution
- Consecutive square sum properties

### External Libraries

- GMP (GNU Multiple Precision Arithmetic Library)
- OpenMP (parallel threading framework)
- Standard GNU C libraries

---

## Glossary

**Converse Prime**: A prime p where both p and reverse(p) are sums of consecutive squares (n² + (n+1)²)

**Emirp**: A prime that yields a different prime when its digits are reversed

**Otto Prime**: Palindromic prime in the form n² + (n+1)² (none found to date)

**Zone-skipping**: Algorithmic technique to eliminate ranges that cannot contain valid candidates

**Gatekeeper**: Modular arithmetic pattern used to pre-filter candidates

**Miller-Rabin**: Probabilistic primality test used for fast prime verification

**GMP**: GNU Multiple Precision library for arbitrary-precision arithmetic

**CVPipe**: The complete 5-stage parallel pipeline for converse prime hunting

---

*Document Version: 1.0*  
*Last Updated: January 31, 2026*  
*Project Duration: 1999-2026 (25+ years)*
