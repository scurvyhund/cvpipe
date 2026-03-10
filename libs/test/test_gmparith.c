/* test_gmparith.c
 *
 * Test program for libgmparith.a (GMP Arithmetic Library)
 *
 * Compile: gcc -O2 test_gmparith.c -L../gmparith -lgmparith -lgmp -o test_gmparith
 * Run: ./test_gmparith
 */

#include <stdio.h>
#include <gmp.h>
#include "../gmparith/gmparith.h"

int main(void) {
    mpz_t p, n, verify, disc;
    mpz_init(p);
    mpz_init(n);
    mpz_init(verify);
    mpz_init(disc);

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  libgmparith.a Test Suite (GMP Arithmetic Library)          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* ── Test 1: Known Converse Pair (12641) ─────────────────────── */
    printf("Test 1: Known Converse Prime 12641 = 79² + 80²\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_ui(p, 12641);
    gmp_printf("  p = %Zd\n", p);

    if (gmp_is_consec_sq_sum(p, disc)) {
        printf("  ✓ Is consecutive square sum\n");
        if (gmp_solve_consec_sqr(p, n, disc)) {
            gmp_printf("  ✓ Solved: n = %Zd (expected: 79)\n", n);

            // Verify
            gmp_n_to_prime(verify, n);
            gmp_printf("  ✓ Verify: %Zd² + %Zd² = %Zd\n", n, n, verify);
            printf("  Result:   %s\n\n", mpz_cmp(verify, p) == 0 ? "✓ PASS" : "✗ FAIL");
        } else {
            printf("  ✗ Failed to solve\n  Result:   ✗ FAIL\n\n");
        }
    } else {
        printf("  ✗ Not a consecutive square sum\n  Result:   ✗ FAIL\n\n");
    }

    /* ── Test 2: Known Converse Pair (14621) ─────────────────────── */
    printf("Test 2: Known Converse Prime 14621 = 85² + 86²\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_ui(p, 14621);
    gmp_printf("  p = %Zd\n", p);

    if (gmp_verify_consecutive_squares(p, n)) {
        gmp_printf("  ✓ Verified: n = %Zd (expected: 85)\n", n);
        printf("  Result:   %s\n\n", mpz_cmp_ui(n, 85) == 0 ? "✓ PASS" : "✗ FAIL");
    } else {
        printf("  ✗ Verification failed\n  Result:   ✗ FAIL\n\n");
    }

    /* ── Test 3: Otto Prime (palindrome) ─────────────────────────── */
    printf("Test 3: Otto Prime 181 = 9² + 10²\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_ui(p, 181);
    gmp_printf("  p = %Zd\n", p);

    if (gmp_solve_consec_sqr(p, n, disc)) {
        gmp_printf("  ✓ Solved: n = %Zd (expected: 9)\n", n);
        printf("  Result:   %s\n\n", mpz_cmp_ui(n, 9) == 0 ? "✓ PASS" : "✗ FAIL");
    } else {
        printf("  ✗ Failed to solve\n  Result:   ✗ FAIL\n\n");
    }

    /* ── Test 4: Prime-to-N Conversion ───────────────────────────── */
    printf("Test 4: Prime ↔ N Conversion\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_ui(p, 12641);
    gmp_prime_to_n(n, p);
    gmp_printf("  12641 → n = %Zd (expected: 79)\n", n);

    gmp_n_to_prime(verify, n);
    gmp_printf("  n = 79 → p = %Zd (expected: 12641)\n", verify);
    printf("  Result:   %s\n\n", mpz_cmp(verify, p) == 0 ? "✓ PASS" : "✗ FAIL");

    /* ── Test 5: Digit Counting ──────────────────────────────────── */
    printf("Test 5: Digit Counting\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_str(p, "1000000000000000000000000", 10);  // 10^24
    int digits = gmp_count_digits(p);
    gmp_printf("  p = %Zd\n", p);
    printf("  Digits: %d (expected: 25)\n", digits);
    printf("  Result:   %s\n\n", digits == 25 ? "✓ PASS" : "✗ FAIL");

    /* ── Test 6: Large Number ────────────────────────────────────── */
    printf("Test 6: Large Number (10^25 range)\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_str(p, "10000000000000000000000041", 10);  // ~10^25, ends in 41
    gmp_printf("  p = %Zd\n", p);

    if (gmp_is_consec_sq_sum(p, disc)) {
        if (gmp_solve_consec_sqr(p, n, disc)) {
            gmp_printf("  ✓ Solved: n = %Zd\n", n);
            gmp_n_to_prime(verify, n);
            printf("  Result:   %s\n\n", mpz_cmp(verify, p) == 0 ? "✓ PASS" : "✗ FAIL");
        } else {
            printf("  ✗ Failed to solve\n  Result:   ✗ FAIL\n\n");
        }
    } else {
        printf("  Not a consecutive square sum (expected)\n  Result:   ✓ PASS\n\n");
    }

    /* ── Test 7: Non-consecutive Square ──────────────────────────── */
    printf("Test 7: Non-consecutive Square (should fail)\n");
    printf("───────────────────────────────────────────────────────────────\n");
    mpz_set_ui(p, 12345);
    gmp_printf("  p = %Zd\n", p);
    printf("  Is consec sq: %s (expected: no)\n",
           gmp_is_consec_sq_sum(p, disc) ? "yes" : "no");
    printf("  Result:   %s\n\n",
           !gmp_is_consec_sq_sum(p, disc) ? "✓ PASS" : "✗ FAIL");

    mpz_clear(p);
    mpz_clear(n);
    mpz_clear(verify);
    mpz_clear(disc);

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  All tests complete!                                         ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    return 0;
}
