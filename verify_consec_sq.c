/*
 * verify_consec_sq.c
 *
 * Verify if a number p is a sum of consecutive squares: p = n² + (n+1)²
 * Also checks p ≡ 1 (mod 4) and primality.
 *
 * Compile: gcc -O3 -Wall verify_consec_sq.c -o verify_consec_sq -lgmp
 * Usage:   ./verify_consec_sq <number>
 * Example: ./verify_consec_sq 1276718683571208763470841
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <gmp.h>

#define MR_ROUNDS 25

bool solve_consecutive_squares(mpz_t p, mpz_t n_out) {
    /*
     * For p = n² + (n+1)² = 2n² + 2n + 1
     * Solving for n:
     *   2n² + 2n + (1-p) = 0
     *   n = (-1 + sqrt(2p - 1)) / 2
     *
     * So we need:
     *   1. (2p - 1) to be a perfect square
     *   2. (sqrt(2p-1) - 1) to be even
     */

    mpz_t discriminant, sqrt_d, numerator;
    mpz_init(discriminant);
    mpz_init(sqrt_d);
    mpz_init(numerator);

    // discriminant = 2p - 1
    mpz_mul_ui(discriminant, p, 2);
    mpz_sub_ui(discriminant, discriminant, 1);

    // Check if discriminant is a perfect square
    if (!mpz_perfect_square_p(discriminant)) {
        mpz_clear(discriminant);
        mpz_clear(sqrt_d);
        mpz_clear(numerator);
        return false;
    }

    // sqrt_d = sqrt(discriminant)
    mpz_sqrt(sqrt_d, discriminant);

    // n = (sqrt_d - 1) / 2
    mpz_sub_ui(numerator, sqrt_d, 1);

    // Check if numerator is even
    if (!mpz_divisible_ui_p(numerator, 2)) {
        mpz_clear(discriminant);
        mpz_clear(sqrt_d);
        mpz_clear(numerator);
        return false;
    }

    mpz_fdiv_q_ui(n_out, numerator, 2);

    mpz_clear(discriminant);
    mpz_clear(sqrt_d);
    mpz_clear(numerator);
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        fprintf(stderr, "Example: %s 1276718683571208763470841\n", argv[0]);
        return 1;
    }

    mpz_t p, n, verify;
    mpz_init(p);
    mpz_init(n);
    mpz_init(verify);

    // Parse input
    if (mpz_set_str(p, argv[1], 10) != 0) {
        fprintf(stderr, "Error: Invalid number format\n");
        mpz_clear(p);
        mpz_clear(n);
        mpz_clear(verify);
        return 1;
    }

    printf("\n");
    printf("Consecutive Square Verification\n");
    printf("================================\n");
    gmp_printf("Input:  %Zd\n", p);
    printf("\n");

    // Check p mod 4
    unsigned long mod4 = mpz_fdiv_ui(p, 4);
    printf("p mod 4 = %lu", mod4);
    if (mod4 == 1) {
        printf("  ✓ (required for consecutive squares)\n");
    } else {
        printf("  ✗ (must be 1 for consecutive squares)\n");
    }

    // Check primality
    int prime_result = mpz_probab_prime_p(p, MR_ROUNDS);
    printf("Prime?    ");
    if (prime_result == 2) {
        printf("YES (definitely prime)\n");
    } else if (prime_result == 1) {
        printf("YES (probably prime, %d rounds Miller-Rabin)\n", MR_ROUNDS);
    } else {
        printf("NO (composite)\n");
    }

    // Check consecutive squares
    printf("\n");
    if (solve_consecutive_squares(p, n)) {
        printf("Consecutive square sum: YES\n");
        printf("\n");
        gmp_printf("  n = %Zd\n", n);
        printf("\n");

        // Verify by computing n² + (n+1)²
        mpz_t n_plus_1;
        mpz_init(n_plus_1);
        mpz_add_ui(n_plus_1, n, 1);

        mpz_mul(verify, n, n);               // verify = n²
        mpz_addmul(verify, n_plus_1, n_plus_1);  // verify += (n+1)²

        gmp_printf("  %Zd² + %Zd² = %Zd\n", n, n_plus_1, verify);
        mpz_clear(n_plus_1);

        printf("\n");
        if (mpz_cmp(verify, p) == 0) {
            printf("  Verification: ✓ CONFIRMED\n");
        } else {
            printf("  Verification: ✗ MISMATCH (this should never happen)\n");
        }
    } else {
        printf("Consecutive square sum: NO\n");
        printf("  This number cannot be expressed as n² + (n+1)²\n");
    }

    printf("\n");
    printf("================================\n");

    mpz_clear(p);
    mpz_clear(n);
    mpz_clear(verify);

    return 0;
}
