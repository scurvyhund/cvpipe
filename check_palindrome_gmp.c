/*
 * check_palindrome_gmp.c
 * 
 * GMP version - handles numbers beyond 2^64
 * Read prime files and check for palindromic primes (otto primes)
 * 
 * Input:  primes01.dat through primes16.dat
 * Output: otto_primes.dat
 * 
 * Compile: gcc -O3 -fopenmp -Wall check_palindrome_gmp.c -o check_palindrome_gmp -lgmp
 * Usage: ./check_palindrome_gmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <gmp.h>

#define NUM_THREADS 16

/* Check if a number (as string) is a palindrome */
static bool is_palindrome_str(const char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        if (str[i] != str[len - 1 - i]) {
            return false;
        }
    }
    return true;
}

/* Solve n^2 + (n+1)^2 = p for n, return true if exact solution exists */
static bool solve_consecutive_squares(mpz_t p, mpz_t n_out) {
    // n^2 + (n+1)^2 = p
    // 2n^2 + 2n + 1 = p
    // 2n^2 + 2n + (1-p) = 0
    // Using quadratic formula: n = (-2 ± sqrt(4 - 8(1-p))) / 4
    //                            = (-2 ± sqrt(4 + 8p - 8)) / 4
    //                            = (-2 ± sqrt(8p - 4)) / 4
    //                            = (-1 ± sqrt(2p - 1)) / 2
    
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
    char prime[256];
    char n_value[256];
} otto_prime_t;

int main(void) {
    time_t start_time = time(NULL);
    
    printf("\n");
    printf("CVPipe Stage 3.5: Otto Prime Hunter (GMP Edition)\n\n");
    printf("\tInput:    primes01.dat through primes16.dat\n");
    printf("\tOutput:   otto_primes.dat\n");
    printf("\tFilter:   Palindromic primes (4n+1, consecutive squares)\n");
    printf("\tThreads:  %d\n", NUM_THREADS);
    printf("\tRange:    Unlimited (supports u128 and beyond)\n");
    printf("==============================================================\n\n");
    
    // Collect otto primes from all threads
    otto_prime_t *all_ottos = malloc(100 * sizeof(otto_prime_t));
    
    if (!all_ottos) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    int otto_count = 0;
    
    uint64_t total_primes = 0;
    uint64_t total_palindromes = 0;
    
    omp_set_num_threads(NUM_THREADS);
    
    #pragma omp parallel reduction(+:total_primes,total_palindromes)
    {
        int tid = omp_get_thread_num();
        char infile[32];
        
        // GMP variables for this thread
        mpz_t prime, n;
        mpz_init(prime);
        mpz_init(n);
        
        // Local otto buffer for this thread
        otto_prime_t local_ottos[100];
        int local_count = 0;
        
        // Open input file
        snprintf(infile, sizeof(infile), "primes%02d.dat", tid + 1);
        FILE *fp = fopen(infile, "r");
        if (!fp) {
            fprintf(stderr, "Thread %d: cannot open %s\n", tid, infile);
            mpz_clear(prime);
            mpz_clear(n);
            exit(1);
        }
        
        uint64_t primes_read = 0;
        uint64_t palindromes_found = 0;
        
        // Read primes and check for palindromes
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            // Remove trailing newline
            line[strcspn(line, "\n")] = 0;
            
            primes_read++;
            
            // Check if palindrome
            if (is_palindrome_str(line)) {
                palindromes_found++;
                
                // Parse as GMP integer
                if (mpz_set_str(prime, line, 10) == 0) {
                    // Solve for n
                    if (solve_consecutive_squares(prime, n)) {
                        // Found an otto prime!
                        if (local_count < 100) {
                            strncpy(local_ottos[local_count].prime, line, 255);
                            local_ottos[local_count].prime[255] = '\0';
                            
                            mpz_get_str(local_ottos[local_count].n_value, 10, n);
                            local_count++;
                        }
                    }
                }
            }
        }
        
        fclose(fp);
        mpz_clear(prime);
        mpz_clear(n);
        
        printf("[Thread %2d] Complete: %lu primes, %lu palindromes\n",
               tid, primes_read, palindromes_found);
        
        total_primes += primes_read;
        total_palindromes += palindromes_found;
        
        // Add local ottos to global list (critical section)
        #pragma omp critical
        {
            for (int i = 0; i < local_count && otto_count < sizeof(otto_count); i++) {
                all_ottos[otto_count++] = local_ottos[i];
            }
        }
    }
    
    // Write all otto primes to output file
    FILE *out = fopen("otto_primes.dat", "w");
    if (!out) {
        fprintf(stderr, "Cannot create otto_primes.dat\n");
        free(all_ottos);
        return 1;
    }
    
    printf("\n");
    printf("OTTO PRIMES FOUND (Palindromic)\n\n");
    
    for (int i = 0; i < otto_count; i++) {
        fprintf(out, "%s %s\n", all_ottos[i].prime, all_ottos[i].n_value);
        printf("\t%s = %s^2 + (%s+1)^2\n", 
               all_ottos[i].prime, 
               all_ottos[i].n_value,
               all_ottos[i].n_value);
    }
    printf("=============================================================\n\n");
    
    fclose(out);
    free(all_ottos);
    
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("PALINDROME CHECK COMPLETE\n");
    printf("\n\tTotal primes:      %12lu\n", total_primes);
    printf("\tOtto primes:       %12d \n", otto_count);
    printf("\tElapsed time:      %12.0f seconds\n", elapsed);
    printf("=============================================================\n");
    
    return 0;
}
