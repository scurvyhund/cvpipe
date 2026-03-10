/* gmp_conversion.c
 *
 * Conversion between prime p and n for consecutive square formula
 * p = n² + (n+1)² = 2n² + 2n + 1
 */

#include "gmparith.h"

void gmp_prime_to_n(mpz_t n, mpz_t prime) {
    mpz_t two_p_minus_1, sq;
    mpz_init(two_p_minus_1);
    mpz_init(sq);

    /* Compute 2p - 1 */
    mpz_mul_ui(two_p_minus_1, prime, 2);
    mpz_sub_ui(two_p_minus_1, two_p_minus_1, 1);

    /* Take square root */
    mpz_sqrt(sq, two_p_minus_1);

    /* n = (sqrt(2p-1) - 1) / 2 */
    mpz_sub_ui(sq, sq, 1);
    mpz_fdiv_q_ui(n, sq, 2);

    mpz_clear(two_p_minus_1);
    mpz_clear(sq);
}

void gmp_n_to_prime(mpz_t prime, mpz_t n) {
    /* p = 2n² + 2n + 1 */
    mpz_mul(prime, n, n);              /* prime = n² */
    mpz_mul_ui(prime, prime, 2);       /* prime = 2n² */
    mpz_addmul_ui(prime, n, 2);        /* prime += 2n */
    mpz_add_ui(prime, prime, 1);       /* prime += 1 */
}
