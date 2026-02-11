/*
 * filter_primes_gmp.c
 * 
 * GMP version - handles numbers beyond 2^64
 * Read candidate files (p01.dat - p16.dat) and filter for primes
 * 
 * Input:  p01.dat through p16.dat (candidates as text, one per line)
 * Output: primes01.dat through primes16.dat (only the primes)
 * 
 * Compile: gcc -O3 -fopenmp -Wall filter_primes_gmp.c -o filter_primes_gmp
 *              -lgmp
 *
 * Usage: ./filter_primes_gmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>
#include <string.h>
#include <time.h>
#include <gmp.h>

#define MR_ROUNDS 25    // Miller-Rabin rounds
#define NUM_THREADS 16

typedef struct {
    int thread_id;
    uint64_t candidates_read;
    uint64_t primes_found;
    FILE *fp_in;
    FILE *fp_out;
} thread_data_t;

int main(void) {
    time_t start_time = time(NULL);
    
    printf("\n");
    printf("CVPipe Stage 2: Prime Filter (GMP Edition)\n");
    
    printf("\n\tInput:    p01.dat...p16.dat (sums of consecutive squares)\n");
    printf("\tOutput:   primes01.dat through primes16.dat\n");
    printf("\tMethod:   GMP Miller-Rabin (%d rounds)\n", MR_ROUNDS);
    printf("\tThreads:  %d\n", NUM_THREADS);
    printf("\tRange:    Unlimited (supports u128 and beyond)\n");
    printf("===========================================================\n\n");
    
    omp_set_num_threads(NUM_THREADS);
    
    // Global counters (will be summed from threads)
    uint64_t total_candidates = 0;
    uint64_t total_primes = 0;
    
    #pragma omp parallel reduction(+:total_candidates,total_primes)
    {
        // Get thread number from omp for our td struct member td.thread_id...
        int tid = omp_get_thread_num();

        // Create  thread struct based on our td struct data type... 
        thread_data_t td;

        char infile[32], outfile[32];
        
        // GMP variable for this thread
        mpz_t candidate;
        mpz_init(candidate);
        
        td.thread_id = tid;
        td.candidates_read = 0;
        td.primes_found = 0;
        
        // Open input file
        snprintf(infile, sizeof(infile), "p%02d.dat", tid + 1);
        
        td.fp_in = fopen(infile, "r");
        if (!td.fp_in) {
            fprintf(stderr, "Thread %d: cannot open %s\n", tid, infile);
            mpz_clear(candidate);
            exit(1);
        }
        
        // Open output file for ea. thread...
        snprintf(outfile, sizeof(outfile), "primes%02d.dat", tid + 1);

        td.fp_out = fopen(outfile, "w");
        if (!td.fp_out) {
            fprintf(stderr, "Thread %d: cannot create %s\n", tid, outfile);
            fclose(td.fp_in);
            mpz_clear(candidate);
            exit(1);
        }
        
        // Read candidates and test for primality
        char line[256];  // Increased for larger numbers
        while (fgets(line, sizeof(line), td.fp_in)) {
            // Remove trailing newline
            line[strcspn(line, "\n")] = 0;
            
            // Parse as GMP integer
            if (mpz_set_str(candidate, line, 10) != 0) {
                fprintf(stderr, "Thread %d: invalid number: %s\n", tid, line);
                continue;
            }
            
            td.candidates_read++;
            
            // Miller-Rabin primality test
            if (mpz_probab_prime_p(candidate, MR_ROUNDS) > 0) {
                gmp_fprintf(td.fp_out, "%Zd\n", candidate);
                td.primes_found++;
            }
        }
        
        fclose(td.fp_in);
        fclose(td.fp_out);
        mpz_clear(candidate);
        
        printf("[Thread %2d] Complete: %lu candidates -> %lu primes (%.2f%%)\n",
            tid, td.candidates_read, td.primes_found,td.candidates_read>0 ?
            100.0*td.primes_found/td.candidates_read:0.0);
        
        // Add to global totals
        total_candidates += td.candidates_read;
        total_primes += td.primes_found;
    }
    
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("\nFILTERING COMPLETE\n");
    printf("\n\tTotal candidates:  %12lu\n", total_candidates);
    printf("\tPrimes found:      %12lu\n", total_primes);
    printf("\tPrime density:     %11.2f%%\n",total_candidates > 0 ? 100.0 *
               total_primes / total_candidates : 0.0);

    printf("\tElapsed time:      %12.0f seconds\n", elapsed);
    printf("==============================================================\n\n");
    
    return 0;
}
