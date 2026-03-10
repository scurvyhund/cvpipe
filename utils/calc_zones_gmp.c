/* calc_zones_gmp.c
 *
 * Calculate valid search zones based on proven digit constraints
 * for the formula 2n² + 2n + 1
 *
 * Valid last-two digits: 01, 13, 21, 41, 61, 81 (proven by cycle analysis)
 * Valid first-two digits for emirps: 10, 12, 14, 16, 18, 31 (derived from 
 * symmetry).
 *
 * Compile: gcc -O3 -Wall calc_zones_gmp.c -o calc_zones_gmp -lgmp -lm
 * Usage: ./calc_zones_gmp <min_prime> <max_prime>
 * Examp: ./calc_zones_gmp 1000000000000000000000000 10000000000000000000000000
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <gmp.h>

// Valid first-digit patterns (proven)
const int VALID_PATTERNS[] = {10, 12, 14, 16, 18, 31};
const int NUM_PATTERNS = 6;

// Zone struct for zone calculations...
typedef struct {
    uint64_t n_min;
    uint64_t n_max;
    int pattern;
    int digits;
    char prime_min_str[128];
    char prime_max_str[128];
} Zone;

// Convert prime p to n using: n = floor((-1 + sqrt(2p - 1)) / 2)
void prime_to_n(mpz_t n, mpz_t prime) {
    mpz_t temp, two_p_minus_1;
    mpz_init(temp);
    mpz_init(two_p_minus_1);
    
    // Calculate 2p - 1
    mpz_mul_ui(two_p_minus_1, prime, 2);
    mpz_sub_ui(two_p_minus_1, two_p_minus_1, 1);
    
    // Take square root
    mpz_sqrt(temp, two_p_minus_1);
    
    // Subtract 1 and divide by 2: n = (sqrt(2p-1) - 1) / 2
    mpz_sub_ui(temp, temp, 1);
    mpz_fdiv_q_ui(n, temp, 2);
    
    mpz_clear(temp);
    mpz_clear(two_p_minus_1);
}

// Count decimal digits in a GMP number
int count_digits_gmp(mpz_t num) {
    char *str = mpz_get_str(NULL, 10, num);
    int len = strlen(str);
    free(str);
    return len;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <min_prime> <max_prime>\n", argv[0]);
        fprintf(stderr, "Example: %s 1000000000000000000000000 10000000000000000000000000\n", argv[0]);
        return 1;
    }
    
    mpz_t prime_min, prime_max;
    mpz_init(prime_min);
    mpz_init(prime_max);
    
    if (mpz_set_str(prime_min, argv[1], 10) != 0) {
        fprintf(stderr, "Error: Invalid min_prime\n");
        return 1;
    }
    
    if (mpz_set_str(prime_max, argv[2], 10) != 0) {
        fprintf(stderr, "Error: Invalid max_prime\n");
        return 1;
    }
    
    printf("=================================================================\
            ===============\n");
    printf("Zone Calculator for 2n² + 2n + 1 Search\n");
    printf("=================================================================\
            ===============\n");
    gmp_printf("Prime range: %Zd to %Zd\n", prime_min, prime_max);
    printf("\n");
    
    // Determine digit range
    int d_min = count_digits_gmp(prime_min);
    int d_max = count_digits_gmp(prime_max);
    
    printf("Digit range: %d to %d digits\n", d_min, d_max);
    printf("Valid patterns: 10, 12, 14, 16, 18, 31 (proven from cycle\
            analysis)\n");
    printf("\n");
    
    // Zone strucs storage for zones
    Zone zones[100];  // More than enough for reasonable ranges
    int zone_count = 0;
    
    mpz_t p_zone_min, p_zone_max, p_min, p_max, n_min_z, n_max_z;
    mpz_t power_of_10;
    mpz_init(p_zone_min);
    mpz_init(p_zone_max);
    mpz_init(p_min);
    mpz_init(p_max);
    mpz_init(n_min_z);
    mpz_init(n_max_z);
    mpz_init(power_of_10);
    
    printf("=================================================================\
            ===============\n");
    printf("VALID SEARCH ZONES:\n");
    printf("=================================================================\
            ===============\n");
    
    // For each digit count
    for (int d = d_min; d <= d_max; d++) {
        
        // Calculate 10^(d-2) for this digit range
        mpz_ui_pow_ui(power_of_10, 10, d - 2);
        
        // For each valid pattern
        for (int i = 0; i < NUM_PATTERNS; i++) {
            int pattern = VALID_PATTERNS[i];
            
            /* Calculate prime range for this pattern (first 2 digits of zone).
             * Pattern "12" at d=25 means primes from 12×10^23 to 13×10^23 - 1. 
             */
            mpz_mul_ui(p_zone_min, power_of_10, pattern);
            mpz_mul_ui(p_zone_max, power_of_10, pattern + 1);
            mpz_sub_ui(p_zone_max, p_zone_max, 1);
            
            // Intersect with requested range [prime_min, prime_max]
            if (mpz_cmp(p_zone_min, prime_min) < 0) {
                mpz_set(p_min, prime_min);
            } else {
                mpz_set(p_min, p_zone_min);
            }
            
            if (mpz_cmp(p_zone_max, prime_max) > 0) {
                mpz_set(p_max, prime_max);
            } else {
                mpz_set(p_max, p_zone_max);
            }
            
            // Skip if no overlap
            if (mpz_cmp(p_min, p_max) > 0) {
                continue;
            }
            
            // Convert prime bounds to n bounds
            prime_to_n(n_min_z, p_min);
            prime_to_n(n_max_z, p_max);
            
            // Store zone info
            zones[zone_count].n_min = mpz_get_ui(n_min_z);
            zones[zone_count].n_max = mpz_get_ui(n_max_z);
            zones[zone_count].pattern = pattern;
            zones[zone_count].digits = d;
            
            {
                char *s = mpz_get_str(NULL, 10, p_min);
                snprintf(zones[zone_count].prime_min_str, 128, "%s", s);
                free(s);
                s = mpz_get_str(NULL, 10, p_max);
                snprintf(zones[zone_count].prime_max_str, 128, "%s", s);
                free(s);
            }
            
            // Print zone info
            printf("\nZone %d:\n", zone_count + 1);
            printf("  Pattern:      %d (digits starting with %d)\n", pattern, 
                    pattern);
            printf("  Prime range:  %s to %s\n",
                    zones[zone_count].prime_min_str, 
                    zones[zone_count].prime_max_str);
            printf("  n range:      %lu to %lu\n", 
                    zones[zone_count].n_min, 
                    zones[zone_count].n_max);
            printf("  n values:     %lu\n", 
                    zones[zone_count].n_max - zones[zone_count].n_min + 1);
            
            zone_count++;
        }
    }
    
    printf("\n");
    printf("=================================================================\
            ===============\n");
    printf("SUMMARY:\n");
    printf("=================================================================\
            ===============\n");
    printf("Total valid zones: %d\n", zone_count);
    
    // Calculate total n-values to search
    uint64_t total_n = 0;
    for (int i = 0; i < zone_count; i++) {
        total_n += (zones[i].n_max - zones[i].n_min + 1);
    }
    printf("Total n-values:    %lu\n", total_n);
    
    // Calculate what would have been searched without zone-skip
    mpz_t naive_n_min, naive_n_max;
    mpz_init(naive_n_min);
    mpz_init(naive_n_max);
    prime_to_n(naive_n_min, prime_min);
    prime_to_n(naive_n_max, prime_max);
    uint64_t naive_total = mpz_get_ui(naive_n_max) - mpz_get_ui(naive_n_min) + 1;
    
    printf("\nWithout zone-skip: %lu n-values\n", naive_total);
    printf("With zone-skip:    %lu n-values\n", total_n);
    printf("Reduction:         %.1f%% (%.2fx speedup)\n", 
           100.0 * (naive_total - total_n) / naive_total,
           (double)naive_total / total_n);
    
    printf("\n");
    printf("=================================================================\
            ===============\n");
    printf("ZONE-SKIP CODE TEMPLATE:\n");
    printf("=================================================================\
            ==============\n");
    printf("\n");
    printf("// Add this to our search program:\n");
    printf("typedef struct { uint64_t n_min; uint64_t n_max; int pattern; }\
             SearchZone;\n");
    printf("\n");
    printf("SearchZone zones[] = {\n");
    for (int i = 0; i < zone_count; i++) {
        printf("    { %luULL, %luULL, %d }%s\n",
               zones[i].n_min, zones[i].n_max, zones[i].pattern,
               (i < zone_count - 1) ? "," : "");
    }
    printf("};\n");
    printf("int num_zones = %d;\n", zone_count);
    printf("\n");
    printf("// Search only valid zones:\n");
    printf("for (int z = 0; z < num_zones; z++) {\n");
    printf("    uint64_t n_start = zones[z].n_min;\n");
    printf("    uint64_t n_end = zones[z].n_max;\n");
    printf("    for (uint64_t n = n_start; n <= n_end; n++) {\n");
    printf("        // Your candidate generation code here\n");
    printf("    }\n");
    printf("}\n");
    printf("\n");
    
    // Cleanup
    mpz_clear(prime_min);
    mpz_clear(prime_max);
    mpz_clear(p_zone_min);
    mpz_clear(p_zone_max);
    mpz_clear(p_min);
    mpz_clear(p_max);
    mpz_clear(n_min_z);
    mpz_clear(n_max_z);
    mpz_clear(power_of_10);
    mpz_clear(naive_n_min);
    mpz_clear(naive_n_max);
    
    return 0;
}
