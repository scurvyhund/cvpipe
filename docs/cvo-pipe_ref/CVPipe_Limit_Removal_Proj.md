# CVPipe Cap Removal Project - Progress Tracker
**Date Started:** January 31, 2026  
**Goal:** Remove artificial limits in CVPipe that truncate search results

---

## THE PROBLEM DISCOVERED

### Root Cause
All three checker programs had hard-coded buffer limits that silently truncated results:

1. **check_emirp_gmp.c**: 10,000 emirp global limit + 1,000 per-thread limit
2. **check_palindrome_gmp.c**: 1,000 otto prime global limit + 100 per-thread limit  
3. **check_converse_gmp.c**: 100 converse pair limit

### Evidence
- 10^22, 10^23, and 10^24 searches all reported exactly 10,000 emirps
- Mathematically impossible - emirp count should scale with search size
- Pattern spotted during log file organization

### Impact Assessment
- **CRITICAL:** Emirp searches at 10^22+ are truncated (missing potentially millions of emirps)
- **LOW:** Otto primes unlikely affected (only 3 known, found in small searches)
- **MINIMAL:** Converse pairs unlikely affected (only 1 known in 25 years)

---

## IMPLEMENTATION PLAN

### File 1: check_emirp_gmp.c ⏳ IN PROGRESS
**Strategy:** Stream to file (no buffering)  
**Status:** Code provided, awaiting implementation  
**Changes:**
- Remove `MAX_EMIRPS` define (line 60)
- Remove `all_emirps` buffer
- Remove `local_emirps[1000]` buffers
- Open `emirps.dat` before parallel block
- Write emirps immediately in critical section
- Keep counter for statistics only

**Testing:**
- Compile: `gcc -O3 -fopenmp -Wall check_emirp_gmp.c -o check_emirp_gmp -lgmp`
- Quick test on small dataset
- Verify emirps.dat gets written correctly

---

### File 2: check_palindrome_gmp.c ⏸️ PENDING
**Strategy:** Dynamic allocation with realloc  
**Status:** Awaiting File 1 completion  
**Changes:**
- Remove `MAX_OTTOS` define (line 99)
- Change to `malloc(100 * sizeof(otto_prime_t))`
- Add realloc logic in critical section when capacity reached
- Add `free()` before return

**Testing:**
- Compile: `gcc -O3 -fopenmp -Wall check_palindrome_gmp.c -o check_palindrome_gmp -lgmp`
- Quick test on small dataset
- Verify otto_primes.dat output

---

### File 3: check_converse_gmp.c ⏸️ PENDING
**Strategy:** Dynamic allocation with realloc  
**Status:** Awaiting File 2 completion  
**Changes:**
- Remove `MAX_CONVERSE` define (line 107)
- Change to `malloc(100 * sizeof(converse_pair_t))`
- Add realloc logic when capacity reached
- Add `free()` before return

**Testing:**
- Compile: `gcc -O3 -Wall check_converse_gmp.c -o check_converse_gmp -lgmp`
- Quick test on small dataset
- Verify converse.dat output

---

## SYSTEMATIC VALIDATION PLAN

### Phase 1: Small Scale Validation (Quick Tests)
**Purpose:** Verify code changes work correctly

1. **10^6 search** (~30 seconds)
   - Verify all stages run
   - Check emirp count is reasonable (<10,000)
   - Confirm no caps hit

2. **10^12 search** (~few minutes)
   - Check if emirp count exceeds old 10,000 limit
   - Verify streaming works at moderate scale

### Phase 2: Historical Re-validation
**Purpose:** Determine when cap first affected results

Run complete searches and log emirp counts:
- 10^15
- 10^18
- 10^20
- 10^21
- 10^22 ← Likely first capped search
- 10^23 ← Known to be capped

**Key Question:** At what scale does emirp count first exceed 10,000?

### Phase 3: Large Scale Verification
**Purpose:** Confirm 10^24 results with uncapped pipeline

1. **10^24 re-run** (estimated 12-15 hours)
   - Compare emirp count to previous (should be >> 10,000)
   - Verify converse pair search completeness
   - Document true emirp density

---

## EXPECTED RESULTS

### Emirp Density Estimates
Based on ~1.8% theoretical emirp rate for random primes:

| Search | Total Primes | Old Limit | Expected True Emirps |
|--------|--------------|-----------|----------------------|
| 10^22  | ~17M         | 10,000    | ~15,000 - 150,000    |
| 10^23  | ~171M        | 10,000    | ~150,000 - 1.5M      |
| 10^24  | ~2.75B       | 10,000    | ~1.5M - 15M          |

### Success Criteria
- ✅ Emirp counts scale with search size (no artificial plateaus)
- ✅ No compilation warnings
- ✅ No runtime errors
- ✅ Output files formatted correctly
- ✅ Parallel processing maintains efficiency

---

## RESEARCH IMPACT

### Publication Integrity
- **Before fix:** Cannot claim "exhaustive search" for 10^22+
- **After fix:** Can legitimately claim complete search coverage
- Must document in paper: "Earlier searches used capped buffers; results re-validated with unlimited storage"

### Converse Prime Claim
- **Current:** 12641 ↔ 14621 is only known converse pair through 10^24
- **After fix:** Can confidently state "exhaustive search found no additional pairs through 10^24"

---

## CURRENT STATUS

**Last Updated:** January 31, 2026 - Initial project setup  
**Active Task:** Implementing check_emirp_gmp.c streaming modifications  
**Blocked On:** Nothing  
**Next Steps:** 
1. Implement emirp streaming changes
2. Test compile and quick validation
3. Proceed to palindrome modifications

---

## NOTES & OBSERVATIONS

### Why Visual Pattern Recognition Matters
- Bug discovered by noticing "10000" appearing in multiple log files
- Demonstrates importance of organizing and reviewing computational results
- Human pattern recognition caught what automated tests might miss

### Code Review Insight
- Silent truncation bugs are dangerous in scientific computing
- Always be suspicious of "round number" results (10,000, 1,000, 100)
- Buffer limits should trigger warnings, not silent truncation

---

## COMPLETION CHECKLIST

### Code Modifications
- [ ] check_emirp_gmp.c - streaming implementation
- [ ] check_palindrome_gmp.c - dynamic allocation
- [ ] check_converse_gmp.c - dynamic allocation
- [ ] All files compile without warnings
- [ ] All files tested on small dataset

### Validation Testing  
- [ ] 10^6 validation run
- [ ] 10^12 validation run
- [ ] Historical re-runs (10^15 - 10^23)
- [ ] 10^24 complete re-run
- [ ] Emirp counts show expected scaling

### Documentation
- [ ] Update Makefile comments with realistic time estimates
- [ ] Document cap removal in research notes
- [ ] Note which historical searches need re-validation
- [ ] Add buffer size comments to source code

---

**End of Progress Tracker**
