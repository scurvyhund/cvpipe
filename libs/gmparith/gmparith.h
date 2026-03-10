/* gmparith.h
 *
 * GMP Arithmetic Library for Consecutive Square Operations
 * Requires: GMP library (-lgmp)
 *
 * Functions for working with the consecutive square formula:
 * p = n² + (n+1)² = 2n² + 2n + 1
 */

#ifndef GMPARITH_H
#define GMPARITH_H

#include <gmp.h>
#include <stdbool.h>

/* ────────────────────────────────────────────────────────────────────
 * Digit Counting
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Count decimal digits in a GMP integer
 * @param num GMP integer
 * @return Number of decimal digits (0 returns 1)
 */
int gmp_count_digits(mpz_t num);


/* ────────────────────────────────────────────────────────────────────
 * Prime ↔ N Conversion
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Convert prime p to n using inverse formula
 * For p = n² + (n+1)², computes n = floor((sqrt(2p-1) - 1) / 2)
 *
 * @param n Output: computed n value
 * @param prime Input: prime number p
 */
void gmp_prime_to_n(mpz_t n, mpz_t prime);

/**
 * Convert n to prime p using forward formula
 * Computes p = 2n² + 2n + 1
 *
 * @param prime Output: computed prime
 * @param n Input: n value
 */
void gmp_n_to_prime(mpz_t prime, mpz_t n);


/* ────────────────────────────────────────────────────────────────────
 * Consecutive Square Tests
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Quick check: is p a sum of consecutive squares?
 * Tests if p = n² + (n+1)² for some integer n
 *
 * Method: p ≡ 1 (mod 4) AND 2p-1 is a perfect square
 *
 * @param p Input number to test
 * @param disc Temporary variable for discriminant (caller allocates)
 * @return true if p is consecutive square sum, false otherwise
 */
bool gmp_is_consec_sq_sum(mpz_t p, mpz_t disc);

/**
 * Solve n² + (n+1)² = p for n
 * Returns n if solution exists, false otherwise
 *
 * @param p Input: number to solve for
 * @param n_out Output: solution n (if exists)
 * @param disc Temporary variable (caller allocates)
 * @return true if solution found, false otherwise
 */
bool gmp_solve_consec_sqr(mpz_t p, mpz_t n_out, mpz_t disc);

/**
 * Full verification: solve and verify by recomputing
 * Solves for n and verifies n² + (n+1)² = p
 *
 * @param p Input: number to verify
 * @param n_out Output: solution n (if valid)
 * @return true if valid consecutive square sum, false otherwise
 */
bool gmp_verify_consecutive_squares(mpz_t p, mpz_t n_out);


/* ────────────────────────────────────────────────────────────────────
 * Zone Computation (for search optimization)
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Compute search zones for digit-pattern-based skipping
 * Used in cvpipe zone-skip optimization
 *
 * @param zones Output array of zone structures
 * @param max_zones Maximum number of zones to compute
 * @param prime_min Minimum prime in search range
 * @param prime_max Maximum prime in search range
 * @return Number of zones computed
 */
typedef struct {
    unsigned long n_min;
    unsigned long n_max;
    int pattern;   /* First 2-digit pattern: {10,12,14,16,18,31} */
    int digits;    /* Number of digits in primes in this zone */
} gmp_search_zone_t;

int gmp_compute_zones(gmp_search_zone_t *zones, int max_zones,
                      mpz_t prime_min, mpz_t prime_max);

#endif /* GMPARITH_H */
