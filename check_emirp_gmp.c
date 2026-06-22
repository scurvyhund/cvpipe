/*
 * check_emirp_gmp.c       revised to remove coded limits on emirps, 01.31.26
 * 
 * GMP version - handles numbers beyond 2^64
 * Read prime files and check for emirps (primes whose reverse is also prime)
 * 
 * Input:  primes01.dat through primes16.dat (primes from filter_primes)
 * Output: emirps.dat (emirp pairs found)
 * 
 * Compile: gcc -O3 -fopenmp -Wall check_emirp_gmp.c -o check_emirp_gmp -lgmp
 * Usage: ./check_emirp_gmp
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
#define MR_ROUNDS 25

/* Reverse a string using thread-local buffer - NO MALLOC! */
static char* reverse_string(const char *str) {
    static __thread char reversed[256];  // Thread-safe, per-thread buffer
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        reversed[i] = str[len - 1 - i];
    }
    reversed[len] = '\0';
    return reversed;
}

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

typedef struct {
   char prime[256];
   char reverse[256];
} emirp_pair_t;

int main(void) {
   time_t start_time = time(NULL);
    
   // Open output file ONCE (outside parallel region)
   FILE *fp_out = fopen("emirps.dat", "w");
   if (!fp_out) {
      fprintf(stderr, "Cannot create emirps.dat\n");
      return 1;
   }
    
   uint64_t total_primes = 0;
   uint64_t total_palindromes = 0;
   uint64_t total_emirps = 0;
    
   omp_set_num_threads(NUM_THREADS);
    
   #pragma omp parallel reduction(+:total_primes,total_palindromes,total_emirps)
   {
      int tid = omp_get_thread_num();
      char infile[32];
        
      // GMP variables for this thread
      mpz_t reversed_num;
      mpz_init(reversed_num);
      
      int local_emirps = 0;  // Just a counter now
      
      // Open input file
      snprintf(infile, sizeof(infile), "primes%02d.dat", tid + 1);
      FILE *fp = fopen(infile, "r");

      if (!fp) {
         fprintf(stderr, "Thread %d: cannot open %s\n", tid, infile);
         mpz_clear(reversed_num);
         exit(1);
      }
        
      uint64_t primes_read = 0;
      uint64_t palindromes_found = 0;
        
      // Read primes and check for emirps
      char line[256];
      
      while (fgets(line, sizeof(line), fp)) {
         line[strcspn(line, "\n")] = 0;
         primes_read++;
            
         if (is_palindrome_str(line)) {
            palindromes_found++;
            continue;
         }
            
         char *reversed_str = reverse_string(line);
            
         if (mpz_set_str(reversed_num, reversed_str, 10) == 0) {
            
            if (mpz_probab_prime_p(reversed_num, MR_ROUNDS) > 0) {
               // Found emirp - write immediately
               #pragma omp critical
               {
                  fprintf(fp_out, "%s %s\n", line, reversed_str);
                  fflush(fp_out);  // Ensure it's written
               }
               local_emirps++;
            }
         }
       }
        
       fclose(fp);
       mpz_clear(reversed_num);
        
       printf("[Thread %2d] Complete: %lu primes, %lu palindromes, %d emirps\n",
              tid, primes_read, palindromes_found, local_emirps);
        
       total_primes += primes_read;
       total_palindromes += palindromes_found;
       total_emirps += local_emirps;
    }
    
    fclose(fp_out);
    
    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);
    
    printf("\nEMIRP CHECK COMPLETE\n");
    printf("\n\tTotal primes:      %12lu\n", total_primes);
    printf("\tPalindromes:       %12lu\n", total_palindromes);
    printf("\tEmirps found:      %12lu \n", total_emirps);
    printf("\tElapsed time:      %12.0f seconds\n", elapsed);
    printf("==============================================================\n");
    
    return 0;
}
