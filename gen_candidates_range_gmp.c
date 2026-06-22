/* gen_candidates_range_gmp.c
 * 
 * GMP version - handles numbers beyond 2^64
 * Generate all n^2 + (n+1)^2 values for n in RANGE [start_n, end_n]
 * Each thread handles n ≡ k (mod 16) for k = 0..15
 * Output: p01.dat through p16.dat (text, one number per line)
 * 
 * INCREMENTAL SEARCH VERSION - Continue from previous run!
 * 
 * Compile: gcc -O3 -march=znver2 -fopenmp -Wall gen_candidates_range_gmp.c
 *          -o gen_candidates_range -lgmp
 * Usage: ./gen_candidates_range <start_n> <max_prime>
 * Example: ./gen_candidates_range 223606797750 1000000000000000000000000
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <omp.h>
#include <gmp.h>

#define NUM_THREADS 16
#define BUFFER_SIZE 10000

typedef struct {
   char **buffer;
   size_t count;
   FILE *fp;
   int thread_id;
   uint64_t n_current;
   uint64_t candidates_written;
   bool error_flag;
} thread_data_t;

volatile bool global_error = false;

void flush_buffer(thread_data_t *td) {

   for (size_t i = 0; i < td->count; i++) {
      fprintf(td->fp, "%s\n", td->buffer[i]);
      free(td->buffer[i]);
      td->buffer[i] = NULL;
   }

   td->candidates_written += td->count;
   td->count = 0;
}

bool is_valid_candidate(const char *str) {
    
   int len = strlen(str);
   if (len < 2) return false;
    
   int first_digit = str[0] - '0';
   int second_digit = str[1] - '0';
    
   bool valid_first2 = false;
   
   if (first_digit == 1) {
      if (second_digit == 0 || second_digit == 2 || second_digit == 4 || 
          second_digit == 6 || second_digit == 8) {
          valid_first2 = true;
      }
   } else if (first_digit == 3 && second_digit == 1) {
        valid_first2 = true;
   }
    
   if (!valid_first2) return false;
   
   int last_digit = str[len - 1] - '0';
   int second_last = (len >= 2) ? str[len - 2] - '0' : 0;
   int last2 = second_last * 10 + last_digit;
   
   if ((last_digit == 1 || last_digit == 3) && (last2 % 4 == 1)) {
       return true;
   }
   
   return false;
}

void add_candidate(thread_data_t *td, mpz_t candidate) {
    
   char *str = mpz_get_str(NULL, 10, candidate);
    
   if (is_valid_candidate(str)) {
      td->buffer[td->count++] = str;
  
      if (td->count >= BUFFER_SIZE) {
         flush_buffer(td);
        }
    } else {
        free(str);
    }
}

int main(int argc, char *argv[]) {
   
   if (argc != 3) {
      fprintf(stderr, "Usage: %s <start_n> <max_prime>\n", argv[0]);
      fprintf(stderr, "Example: %s 223606797750 1000000000000000000000000\n",
              argv[0]);
      fprintf(stderr, "\n");
      fprintf(stderr, "INCREMENTAL SEARCH:\n");
      fprintf(stderr, "  Start from where previous run ended\n");
      fprintf(stderr, "  Get start_n from previous log file\n");
      fprintf(stderr, "  (Max N + 1 from previous run)\n");
      return 1;
   }

   // Parse start_n
   mpz_t start_n_z;
   mpz_init(start_n_z);
  
   if (mpz_set_str(start_n_z, argv[1], 10) != 0) {
      fprintf(stderr, "Error: Invalid start_n format\n");
      mpz_clear(start_n_z);
      return 1;
   }

   uint64_t start_n = mpz_get_ui(start_n_z);
   mpz_clear(start_n_z);

   // Parse max_prime
   mpz_t max_prime_z, max_n_z;
   mpz_init(max_prime_z);
   mpz_init(max_n_z);
   
   if (mpz_set_str(max_prime_z, argv[2], 10) != 0) {
      fprintf(stderr, "Error: Invalid max_prime format\n");
      mpz_clear(max_prime_z);
      mpz_clear(max_n_z);
      return 1;
   }
   
   // Calculate max_n = sqrt(max_prime / 2)
   mpz_t temp;
   mpz_init(temp);
   mpz_fdiv_q_ui(temp, max_prime_z, 2);
   mpz_sqrt(max_n_z, temp);
   mpz_clear(temp);
   
   uint64_t max_n = mpz_get_ui(max_n_z);
   
   printf("Consecutive Square Sum Generator - RANGE MODE (GMP Edition)\n");
   printf("===============================================================\n");
   gmp_printf("Max prime: %Zd\n", max_prime_z);
   printf("Start N:   %lu\n", start_n);
   printf("Max N:     %lu\n", max_n);
   printf("Range:     %lu values to search\n", max_n - start_n + 1);
   printf("Threads:   %d\n", NUM_THREADS);
   printf("Formula:   2n² + 2n + 1\n");
   printf("Output:    p01.dat through p16.dat (OVERWRITE MODE)\n");
   printf("===============================================================\n");
   printf("\n");
   
   mpz_clear(max_prime_z);
   mpz_clear(max_n_z);
   
   omp_set_num_threads(NUM_THREADS);
   
   #pragma omp parallel
   {
      int tid = omp_get_thread_num();
      thread_data_t td;
      char filename[32];
      
      mpz_t n_z, candidate;
      mpz_init(n_z);
      mpz_init(candidate);
      
      td.thread_id = tid;
      td.count = 0;
      td.candidates_written = 0;
      td.error_flag = false;
      
      td.buffer = malloc(BUFFER_SIZE * sizeof(char*));
  
      if (!td.buffer) {
         fprintf(stderr, "[Thread %2d] ERROR: malloc failed\n", tid);
         td.error_flag = true;
         global_error = true;
      }
      
      if (!td.error_flag) {
         snprintf(filename, sizeof(filename), "p%02d.dat", tid + 1);
         
         // "w" truncates — overwrites any prior run's output
         td.fp = fopen(filename, "w");
      
         if (!td.fp) {
            fprintf(stderr, "[Thread %2d] ERROR: cannot open %s\n", tid,
                    filename);
            td.error_flag = true;
            global_error = true;
            free(td.buffer);
            td.buffer = NULL;
         } else {
              printf("[Thread %2d] Continuing: n ≡ %d (mod 16) → %s\n", 
                     tid, tid, filename);
         }
      }
      
     if (!td.error_flag && !global_error) {
        // Find first n for this thread >= start_n
        uint64_t first_n = start_n;
        if (first_n % NUM_THREADS != (uint64_t)tid) {
            first_n = start_n + (tid - (start_n % NUM_THREADS) + NUM_THREADS)
            % NUM_THREADS;
        }
         
        for (uint64_t n = first_n; n <= max_n; n += NUM_THREADS) {
         
           if (global_error) break;
            mpz_set_ui(n_z, n);
            mpz_mul(candidate, n_z, n_z);
            mpz_mul_ui(candidate, candidate, 2);
            mpz_addmul_ui(candidate, n_z, 2);
            mpz_add_ui(candidate, candidate, 1);
            
            add_candidate(&td, candidate);
            td.n_current = n;
         }
         
         if (td.count > 0) {
            flush_buffer(&td);
         }
         
         fclose(td.fp);
      }
      
      if (!td.error_flag) {
          printf("[Thread %2d] Complete: %lu candidates written (range)\n",
                 tid, td.candidates_written);
      }
      
      if (td.buffer != NULL) {
          for (size_t i = 0; i < td.count; i++) {
              if (td.buffer[i]) free(td.buffer[i]);
          }
          free(td.buffer);
      }
      
      mpz_clear(n_z);
      mpz_clear(candidate);
   }
    
   if (global_error) {
       fprintf(stderr, "\n!!! ERRORS OCCURRED - Check output files !!!\n");
       return 1;
   }
   
   printf("\n");
   printf("Range Generation Complete\n");
   printf("===============================================================\n");
   printf("Files updated: p01.dat through p16.dat\n");
   printf("Ready for: ./filter_primes\n");
   printf("===============================================================\n");
   
   return 0;
}
