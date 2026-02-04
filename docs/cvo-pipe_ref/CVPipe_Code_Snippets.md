# CVPipe Code Changes - Quick Reference
**Use this for copy-paste during implementation**

---

## FILE 1: check_emirp_gmp.c - STREAMING VERSION

### Full replacement for main() function (starting at line 54)

```c
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
    uint64_t total_emirps = 0;  // Changed to counter only
    
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
            if (!reversed_str) continue;
            
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
            
            free(reversed_str);
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
```

**Compile command:**
```bash
gcc -O3 -fopenmp -Wall check_emirp_gmp.c -o check_emirp_gmp -lgmp
```

---

## FILE 2: check_palindrome_gmp.c - DYNAMIC ALLOCATION

### Change 1: Remove line 99
```c
// DELETE THIS LINE:
#define MAX_OTTOS 1000
```

### Change 2: Replace lines 101-108
```c
// REPLACE:
    #define MAX_OTTOS 1000
    otto_prime_t *all_ottos = malloc(MAX_OTTOS * sizeof(otto_prime_t));
    if (!all_ottos) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    int otto_count = 0;

// WITH:
    // Dynamic otto prime collection
    int otto_capacity = 100;
    otto_prime_t *all_ottos = malloc(otto_capacity * sizeof(otto_prime_t));
    if (!all_ottos) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    int otto_count = 0;
```

### Change 3: Replace lines 173-177 (critical section)
```c
// REPLACE:
        #pragma omp critical
        {
            for (int i = 0; i < local_count && otto_count < MAX_OTTOS; i++) {
                all_ottos[otto_count++] = local_ottos[i];
            }
        }

// WITH:
        // Add local ottos to global list (critical section)
        #pragma omp critical
        {
            for (int i = 0; i < local_count; i++) {
                // Grow array if needed
                if (otto_count >= otto_capacity) {
                    otto_capacity *= 2;
                    otto_prime_t *new_ottos = realloc(all_ottos, otto_capacity * sizeof(otto_prime_t));
                    if (!new_ottos) {
                        fprintf(stderr, "realloc failed at %d ottos\n", otto_count);
                        exit(1);
                    }
                    all_ottos = new_ottos;
                }
                all_ottos[otto_count++] = local_ottos[i];
            }
        }
```

**Compile command:**
```bash
gcc -O3 -fopenmp -Wall check_palindrome_gmp.c -o check_palindrome_gmp -lgmp
```

---

## FILE 3: check_converse_gmp.c - DYNAMIC ALLOCATION

### Change 1: Remove line 107
```c
// DELETE THIS LINE:
#define MAX_CONVERSE 100
```

### Change 2: Replace lines 109-111
```c
// REPLACE:
    converse_pair_t converse_pairs[MAX_CONVERSE];
    int converse_count = 0;

// WITH:
    // Dynamic converse collection
    int converse_capacity = 100;
    converse_pair_t *converse_pairs = malloc(converse_capacity * sizeof(converse_pair_t));
    if (!converse_pairs) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    int converse_count = 0;
```

### Change 3: Replace lines 132-146 (storage logic)
```c
// REPLACE:
        if (p1_is_consec && p2_is_consec) {
            // FOUND A CON-VERSE PAIR!
            if (converse_count < MAX_CONVERSE) {
                strncpy(converse_pairs[converse_count].p1, p1_str, 255);
                strncpy(converse_pairs[converse_count].p2, p2_str, 255);
                converse_pairs[converse_count].p1[255] = '\0';
                converse_pairs[converse_count].p2[255] = '\0';
                
                mpz_get_str(converse_pairs[converse_count].n1, 10, n1);
                mpz_get_str(converse_pairs[converse_count].n2, 10, n2);
                
                converse_count++;
            }
        }

// WITH:
        if (p1_is_consec && p2_is_consec) {
            // FOUND A CON-VERSE PAIR!
            // Grow array if needed
            if (converse_count >= converse_capacity) {
                converse_capacity *= 2;
                converse_pair_t *new_pairs = realloc(converse_pairs, 
                                                     converse_capacity * sizeof(converse_pair_t));
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
```

### Change 4: Add cleanup before final return (around line 186)
```c
// ADD before "return 0;":
    free(converse_pairs);
```

**Compile command:**
```bash
gcc -O3 -Wall check_converse_gmp.c -o check_converse_gmp -lgmp
```

---

## TESTING CHECKLIST

After each file modification:
- [ ] Compile with no warnings
- [ ] Run quick test on small dataset
- [ ] Verify output files created correctly
- [ ] Check emirp/otto/converse counts are reasonable

---
