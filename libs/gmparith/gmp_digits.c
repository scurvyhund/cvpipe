/* gmp_digits.c
 *
 * Digit counting for GMP integers
 */

#include "gmparith.h"

int gmp_count_digits(mpz_t num) {
    if (mpz_sgn(num) == 0) return 1;
    return (int)mpz_sizeinbase(num, 10);
}
