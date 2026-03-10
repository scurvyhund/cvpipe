/* gmp_consecutive.c
 *
 * Consecutive square sum testing and solving
 * p = n² + (n+1)² = 2n² + 2n + 1
 */

#include "gmparith.h"

bool gmp_is_consec_sq_sum(mpz_t p, mpz_t disc) {
    /* Quick filter: all consecutive square sums ≡ 1 (mod 4) */
    if (mpz_fdiv_ui(p, 4) != 1) {
        return false;
    }

    /* Compute discriminant: 2p - 1 */
    mpz_mul_ui(disc, p, 2);
    mpz_sub_ui(disc, disc, 1);

    /* Check if discriminant is a perfect square */
    return mpz_perfect_square_p(disc) != 0;
}

bool gmp_solve_consec_sqr(mpz_t p, mpz_t n_out, mpz_t disc) {
    /* Compute discriminant: 2p - 1 */
    mpz_mul_ui(disc, p, 2);
    mpz_sub_ui(disc, disc, 1);

    /* Must be a perfect square */
    if (!mpz_perfect_square_p(disc)) {
        return false;
    }

    /* Compute sqrt(discriminant) - reuse disc as temp */
    mpz_t sqrt_d;
    mpz_init(sqrt_d);
    mpz_sqrt(sqrt_d, disc);

    /* n = (sqrt_d - 1) / 2 */
    mpz_sub_ui(sqrt_d, sqrt_d, 1);

    /* Check if (sqrt_d - 1) is even */
    if (!mpz_divisible_ui_p(sqrt_d, 2)) {
        mpz_clear(sqrt_d);
        return false;
    }

    mpz_fdiv_q_ui(n_out, sqrt_d, 2);
    mpz_clear(sqrt_d);
    return true;
}

bool gmp_verify_consecutive_squares(mpz_t p, mpz_t n_out) {
    mpz_t disc, verify, n_plus_1;
    mpz_init(disc);
    mpz_init(verify);
    mpz_init(n_plus_1);

    /* Solve for n */
    if (!gmp_solve_consec_sqr(p, n_out, disc)) {
        mpz_clear(disc);
        mpz_clear(verify);
        mpz_clear(n_plus_1);
        return false;
    }

    /* Verify by computing n² + (n+1)² */
    mpz_add_ui(n_plus_1, n_out, 1);
    mpz_mul(verify, n_out, n_out);                /* verify = n² */
    mpz_addmul(verify, n_plus_1, n_plus_1);       /* verify += (n+1)² */

    /* Check if matches original p */
    bool valid = (mpz_cmp(verify, p) == 0);

    mpz_clear(disc);
    mpz_clear(verify);
    mpz_clear(n_plus_1);
    return valid;
}
