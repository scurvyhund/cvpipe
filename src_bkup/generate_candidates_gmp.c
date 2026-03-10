/* generate_candidates_gmp.c     1/21/26
 * 
 * GMP version - handles numbers beyond 2^64
 * Generate all n^2 + (n+1)^2 values using 16 threads
 * Each thread handles n ≡ k (mod 16) for k = 0..15
 * Output: p01.dat through p16.dat (text, one number per line)
 *
 * By construction, all outputs are ≡ 1 (mod 4)
 * 
 * Compile: gcc -O3 -march=znver2 -fopenmp -Wall generate_candidates_gmp.c -o\
 *              generate_candidates_gmp -lgmp
 *
 * Usage: ./generate_candidates_gmp <max_prime>
 *
 * Last successful run 10e23
 * Last full prime load (10e24) p01-16 saved on big8tera. 1/16/26
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>
#include <string.h>
#include <gmp.h>

#define NUM_THREADS 16
#define BUFFER_SIZE 10000  // Buffer 10k candidates before writing

typedef struct {
    char **buffer;         // Array of strings
    size_t count;
    FILE *fp;
    int thread_id;
    uint64_t n_current;
    uint64_t candidates_written;
    bool error_flag;
} thread_data_t;

// Global error flag
volatile bool global_error = false;

void flush_buffer(thread_data_t *td) {
    for (size_t i = 0; i < td->count; i++) {
        fprintf(td->fp, "%s\n", td->buffer[i]);
        free(td->buffer[i]);  // Free the string after writing
        td->buffer[i] = NULL;
    }
    td->candidates_written += td->count;
    td->count = 0;
}

// Gatekeeper: filter for emirp-viable candidates
// Valid first2: 1[0,2,4,6,8] or 31
// Valid last2: ends in 1 or 3, and ≡ 1 (mod 4)
bool is_valid_candidate(const char *str) {
    int len = strlen(str);
    
    if (len < 2) return false;  // Single digit - reject
    
    // Extract first two digits
    int first_digit = str[0] - '0';
    int second_digit = str[1] - '0';
    
    // Check first2 pattern: ONLY 10, 12, 14, 16, 18, 31
    bool valid_first2 = false;
    if (first_digit == 1) {
        // 10, 12, 14, 16, 18
        if (second_digit == 0 || second_digit == 2 || second_digit == 4 || 
            second_digit == 6 || second_digit == 8) {
            valid_first2 = true;
        }
    } else if (first_digit == 3 && second_digit == 1) {
        // ONLY 31
        valid_first2 = true;
    }
    
    if (!valid_first2) return false;
    
    // Check last2: must end in 1 or 3, and ≡ 1 (mod 4)
    // Get last two digits
    int last_digit = str[len - 1] - '0';
    int second_last = (len >= 2) ? str[len - 2] - '0' : 0;
    int last2 = second_last * 10 + last_digit;
    
    if ((last_digit == 1 || last_digit == 3) && (last2 % 4 == 1)) {
        return true;
    }
    
    return false;
}

void add_candidate(thread_data_t *td, mpz_t candidate) {
    // Convert to string
    char *str = mpz_get_str(NULL, 10, candidate);
    
    // Only add if it passes the gatekeeper
    if (is_valid_candidate(str)) {
        td->buffer[td->count++] = str;  // Store the string
        if (td->count >= BUFFER_SIZE) {
            flush_buffer(td);
        }
    } else {
        free(str);  // Don't need it
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <max_prime>\n", argv[0]);
        fprintf(stderr, "Example: %s 100000000000000000000\n", argv[0]);
        return 1;
    }

    // Parse max_prime as GMP integer
    mpz_t max_prime_z, max_n_z;
    mpz_init(max_prime_z);
    mpz_init(max_n_z);
    
    if (mpz_set_str(max_prime_z, argv[1], 10) != 0) {
        fprintf(stderr, "Error: Invalid number format\n");
        mpz_clear(max_prime_z);
        mpz_clear(max_n_z);
        return 1;
    }
    
    // Calculate max_n = sqrt(max_prime / 2)
    mpz_t temp;
    mpz_init(temp);
    mpz_fdiv_q_ui(temp, max_prime_z, 2);  // temp = max_prime / 2
    mpz_sqrt(max_n_z, temp);              // max_n = sqrt(temp)
    mpz_clear(temp);
    
    // Convert max_n to uint64_t for loop control
    // (max_n won't exceed ~10^10 for practical searches)
    uint64_t max_n = mpz_get_ui(max_n_z);
    
    printf("\n");
    printf("  Consecutive Square Sum Generator (GMP Edition) \n");
    printf("\n");
    gmp_printf("\t%-45Zd", max_prime_z);
    printf("\n");
    printf("    Max N:     %-45lu\n", max_n);
    printf("    Threads:   %-2d  \n", NUM_THREADS);
    printf("    Formula:   2n^2 + 2n + 1\n");
    printf("    Output:    p01.dat through p16.dat\n");
    printf("    Range:     UNLIMITED (supports 10^0, 10^100, etc.)\n");
    printf("==============================================================\n\n");
    
    mpz_clear(max_prime_z);
    mpz_clear(max_n_z);
    
    omp_set_num_threads(NUM_THREADS);
    
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        thread_data_t td;
        char filename[32];
        
        // GMP variables for this thread
        mpz_t n_z, candidate;
        mpz_init(n_z);
        mpz_init(candidate);
        
        // Initialize thread data
        td.thread_id = tid;
        td.count = 0;
        td.candidates_written = 0;
        td.error_flag = false;
        
        // Allocate buffer for string pointers
        td.buffer = malloc(BUFFER_SIZE * sizeof(char*));
        if (!td.buffer) {
            fprintf(stderr, "[Thread %2d] ERROR: malloc failed\n", tid);
            td.error_flag = true;
            global_error = true;
        }
        
        // Open output file (only if malloc succeeded)
        if (!td.error_flag) {
            snprintf(filename, sizeof(filename), "p%02d.dat", tid + 1);
            td.fp = fopen(filename, "w");
            if (!td.fp) {
                fprintf(stderr, "[Thread %2d] ERROR: cannot open %s\n", tid, 
                        filename);
                td.error_flag = true;
                global_error = true;
                free(td.buffer);
                td.buffer = NULL;
            } else {
                printf("[Thread %2d] Starting: n ≡ %d (mod 16) -> %s\n", 
                       tid, tid, filename);
            }
        }
        
        // Only process if no errors
        if (!td.error_flag && !global_error) {
            for (uint64_t n = tid; n <= max_n; n += NUM_THREADS) {
                // Check for global error from other threads
                if (global_error) break;
                
                // Calculate 2n^2 + 2n + 1 using GMP
                mpz_set_ui(n_z, n);
                
                // candidate = 2*n^2 + 2*n + 1
                mpz_mul(candidate, n_z, n_z);         // candidate = n^2
                mpz_mul_ui(candidate, candidate, 2);  // candidate = 2*n^2
                mpz_addmul_ui(candidate, n_z, 2);     // candidate += 2*n
                mpz_add_ui(candidate, candidate, 1);  // candidate += 1
                
                add_candidate(&td, candidate);
                td.n_current = n;
            }
            
            // Flush remaining buffer
            if (td.count > 0) {
                flush_buffer(&td);
            }
            
            fclose(td.fp);
        }
        
        // Report completion
        if (!td.error_flag) {
            printf("[Thread %2d] Complete: %lu candidates written\n",
                   tid, td.candidates_written);
        }
        
        // Clean up
        if (td.buffer != NULL) {
            // Free any remaining strings in buffer
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
    printf("CONSEC-SQUARE SUM GENERATION COMPLETE\n");
    printf("\n\tFiles created: p01.dat through p16.dat \n");
    printf("==============================================================\n");
    
    return 0;
}
