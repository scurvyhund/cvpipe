/*
 * check_converse_gmp.c
 * 
 * GMP version - handles numbers beyond 2^64
 * Read emirp pairs and check for con-verse primes
 * (Both p and reverse(p) are sums of consecutive squares)
 * 
 * Input:  emirps.dat
 * Output: converse.dat
 * 
 * Compile: gcc -O3 -Wall check_converse_gmp.c -o check_converse_gmp -lgmp
 * Usage: ./check_converse_gmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <gmp.h>

/* Solve n^2 + (n+1)^2 = p for n, return true if exact solution exists */
static bool solve_consecutive_squares(mpz_t p, mpz_t n_out) {
    // n^2 + (n+1)^2 = p
    // 2n^2 + 2n + 1 = p
    // Using quadratic formula: n = (-1 + sqrt(2p - 1)) / 2
    
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
    
    // n = (-1 + sqrt_d) / 2
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

typedef struct {
    char p1[256];
    char p2[256];
    char n1[256];
    char n2[256];
} converse_pair_t;

int main(void) {
    time_t start_time = time(NULL);
    
    printf("\nCVPipe Stage 4: CON-VERSE CHECKER (GMP Edition)\n");
    printf("\n\tInput:    emirps.dat\n");
    printf("\tOutput:   converse.dat\n");
    printf("\tChecking: Both p and reverse(p) are n^2 + (n+1)^2\n");
    printf("\tRange:    Unlimited (supports u128 and beyond)\n");
    printf("=============================================================\n");
    
    // Open input file
    FILE *fp_in = fopen("emirps.dat", "r");
    if (!fp_in) {
        fprintf(stderr, "Error: cannot open emirps.dat\n");
        fprintf(stderr, "Make sure to run check_emirp first!\n");
        return 1;
    }
    
    // Open output file
    FILE *fp_out = fopen("converse.dat", "w");
    if (!fp_out) {
        fprintf(stderr, "Error: cannot create converse.dat\n");
        fclose(fp_in);
        return 1;
    }
    
    printf("\nProcessing emirp pairs...\n\n");
    
    // GMP variables
    mpz_t p1, p2, n1, n2;
    mpz_init(p1);
    mpz_init(p2);
    mpz_init(n1);
    mpz_init(n2);
   
    // Dynamic converse collection...
    int converse_capacity = 100;

    converse_pair_t *converse_pairs = malloc(converse_capacity * sizeof(converse_pair_t));

    if (!converse_pairs) {
       fprintf(stderr, "malloc failed\n");
       return 1;
    }
    int converse_count = 0;

    uint64_t emirps_checked = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), fp_in)) {
        // Parse two numbers from line
        char p1_str[256], p2_str[256];
        if (sscanf(line, "%255s %255s", p1_str, p2_str) != 2) {
            continue;
        }
        
        emirps_checked++;
        
        // Parse as GMP integers, ret 0 is success...
        if (mpz_set_str(p1, p1_str, 10) != 0) continue;
        if (mpz_set_str(p2, p2_str, 10) != 0) continue;

        // if p1 > p2 return > 0...
        if (mpz_cmp(p1, p2) > 0) continue;

        // Check if BOTH can be expressed as n^2 + (n+1)^2
        bool p1_is_consec = solve_consecutive_squares(p1, n1);
        bool p2_is_consec = solve_consecutive_squares(p2, n2);

        if (p1_is_consec && p2_is_consec) {
           // FOUND A CON-VERSE PAIR!
           // Grow array if needed
           if (converse_count >= converse_capacity) {
              converse_capacity *= 2;
              converse_pair_t *new_pairs = realloc(converse_pairs, converse_capacity * sizeof(converse_pair_t));
              
              if (!new_pairs) {
                 fprintf(stderr, "realloc failed at %d pairs\n", converse_count);
                 fclose(fp_in);
                 fclose(fp_out);
                 return 1;
              }
              converse_pairs = new_pairs;
           }

           strncpy(converse_pairs[converse_count].p1, p1_str, 255);
           strncpy(converse_pairs[converse_count].p2, p2_str, 255);
           converse_pairs[converse_count].p1[255] = '\0';
           converse_pairs[converse_count].p2[255] = '\0';

           mpz_get_str(converse_pairs[converse_count].n1, 10, n1);
           mpz_get_str(converse_pairs[converse_count].n2, 10, n2);

          converse_count++;
       }
   }
    
    fclose(fp_in);
    
    // Write results
    printf("\n");
    for (int i = 0; i < converse_count; i++) {
        printf("CON-VERSE PRIME FOUND #%d \n", i+1);
        printf("\n");
        printf("\t%s <==> %s\n", 
               converse_pairs[i].p1, converse_pairs[i].p2);
        
        printf("\n");
        printf("\t%s = %s^2 + (%s+1)^2", 
               converse_pairs[i].p1, 
               converse_pairs[i].n1,
               converse_pairs[i].n1);

        printf("\n\t%s = %s^2 + (%s+1)^2", 
               converse_pairs[i].p2, 
               converse_pairs[i].n2,
               converse_pairs[i].n2);
        printf("\n=============================================================\n\n");
        
        // Write to file
        fprintf(fp_out, "%s %s %s %s\n", converse_pairs[i].p1, converse_pairs[i].p2,
                converse_pairs[i].n1, converse_pairs[i].n2);
    }
    
    fclose(fp_out);
    
    mpz_clear(p1);
    mpz_clear(p2);
    mpz_clear(n1);
    mpz_clear(n2);
    
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("CON-VERSE CHECK COMPLETE\n");
    printf("\n\tEmirp pairs checked:  %12lu\n", emirps_checked);
    printf("\tCON-VERSE FOUND:      %12d\n", converse_count);
    printf("\tElapsed time:         %12.0f seconds\n", elapsed);
    printf("=============================================================\n");
    
    if (converse_count == 1) {
        printf("\n");
        printf("EXACTLY ONE CON-VERSE PAIR FOUND!\n");
        printf("\n\tThis may be the only con-verse prime pair!\n");
        printf("\n=============================================================\n");
    } else if (converse_count == 0) {
        printf("\n");
        printf("\tNo con-verse pairs found in this range.\n");
        printf("\n=============================================================");
    } else {
        printf("\n");
        printf("MULTIPLE CON-VERSE PAIRS FOUND!\n");
        printf("\n\tThis is a significant discovery!\n");
        printf("===========================================================\n\n");
    }
   
    free(converse_pairs);
    return 0;
}
