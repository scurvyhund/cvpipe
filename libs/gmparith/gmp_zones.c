/* gmp_zones.c
 *
 * Zone computation for digit-pattern-based search optimization
 * Implements the zone-skip algorithm for consecutive square searches
 */

#include "gmparith.h"

/* Valid first-2-digit patterns (proven from cycle analysis) */
static const int VALID_PATTERNS[] = {10, 12, 14, 16, 18, 31};
static const int NUM_PATTERNS = 6;

int gmp_compute_zones(gmp_search_zone_t *zones, int max_zones,
                      mpz_t prime_min, mpz_t prime_max) {
    mpz_t pwr_of_10, p_zone_min, p_zone_max, p_min, p_max;
    mpz_t n_min_z, n_max_z;

    mpz_init(pwr_of_10);
    mpz_init(p_zone_min);
    mpz_init(p_zone_max);
    mpz_init(p_min);
    mpz_init(p_max);
    mpz_init(n_min_z);
    mpz_init(n_max_z);

    /* Determine digit range */
    int d_min = gmp_count_digits(prime_min);
    int d_max = gmp_count_digits(prime_max);

    /* Patterns require at least 2 digits */
    if (d_min < 2) d_min = 2;

    int zone_count = 0;

    /* For each digit count */
    for (int d = d_min; d <= d_max && zone_count < max_zones; d++) {
        /* Compute 10^(d-2) */
        mpz_ui_pow_ui(pwr_of_10, 10, (unsigned long)(d - 2));

        /* For each valid pattern */
        for (int i = 0; i < NUM_PATTERNS && zone_count < max_zones; i++) {
            int pattern = VALID_PATTERNS[i];

            /* Prime sub-range for this pattern at d digits:
             * [pattern * 10^(d-2), (pattern+1) * 10^(d-2) - 1] */
            mpz_mul_ui(p_zone_min, pwr_of_10, pattern);
            mpz_mul_ui(p_zone_max, pwr_of_10, pattern + 1);
            mpz_sub_ui(p_zone_max, p_zone_max, 1);

            /* Intersect with user range [prime_min, prime_max] */
            mpz_set(p_min, mpz_cmp(p_zone_min, prime_min) < 0 ?
                    prime_min : p_zone_min);
            mpz_set(p_max, mpz_cmp(p_zone_max, prime_max) > 0 ?
                    prime_max : p_zone_max);

            /* Skip if no overlap */
            if (mpz_cmp(p_min, p_max) > 0) {
                continue;
            }

            /* Convert prime bounds to n bounds */
            gmp_prime_to_n(n_min_z, p_min);
            gmp_prime_to_n(n_max_z, p_max);

            /* Store zone */
            zones[zone_count].n_min   = mpz_get_ui(n_min_z);
            zones[zone_count].n_max   = mpz_get_ui(n_max_z);
            zones[zone_count].pattern = pattern;
            zones[zone_count].digits  = d;
            zone_count++;
        }
    }

    mpz_clear(pwr_of_10);
    mpz_clear(p_zone_min);
    mpz_clear(p_zone_max);
    mpz_clear(p_min);
    mpz_clear(p_max);
    mpz_clear(n_min_z);
    mpz_clear(n_max_z);

    return zone_count;
}
