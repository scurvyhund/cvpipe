/* cvpipe.c — Merged CVPipe: Con-verse Prime Pipeline (single-pass, no disk I/O)
 *
 * Combines all 5 stages into one in-memory pipeline:
 *   Stage 1: Generate candidates (2n² + 2n + 1)
 *   Stage 2: Miller-Rabin primality test (25 rounds)
 *   Stage 3: Emirp detection (reverse is also prime)
 *   Stage 3.5: Palindromic prime detection (otto primes)
 *   Stage 4: Converse pair detection (both are n² + (n+1)²)
 *
 * Usage:
 *   ./cvpipe <max_prime>              Full run from n=0
 *   ./cvpipe <start_n> <max_prime>    Continue from start_n
 *
 * Compile: gcc -O3 -march=znver2 -mtune=znver2 -fopenmp -Wall cvpipe.c -o cvpipe -lgmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <gmp.h>

#define NUM_THREADS 16
#define MR_ROUNDS 25
#define PROGRESS_INTERVAL 10000000  /* Thread 0 reports every 10M n-values */

/* Per-thread statistics */
typedef struct {
    uint64_t candidates_generated;
    uint64_t gatekeeper_passed;
    uint64_t palindromes_found;
    uint64_t consec_sq_passed;
    uint64_t mr_tests;
    uint64_t primes_found;
    uint64_t emirps_found;
    uint64_t converse_found;
    uint64_t otto_found;
} thread_stats_t;

/* ── Gatekeeper: filter for emirp-viable candidates ──────────────────
 * Valid first2: 1[0,2,4,6,8] or 31
 * Valid last2: ends in 1 or 3, and ≡ 1 (mod 4)
 * From generate_candidates_gmp.c:57-92
 */
static bool is_valid_candidate(const char *str, int len) {
    if (len < 2) return false;

    int first_digit  = str[0] - '0';
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

    int last_digit   = str[len - 1] - '0';
    int second_last  = str[len - 2] - '0';
    int last2 = second_last * 10 + last_digit;

    return (last_digit == 1 || last_digit == 3) && (last2 % 4 == 1);
}

/* ── Quick pre-filter: is p a sum of consecutive squares? ────────────
 * p = n² + (n+1)² = 2n² + 2n + 1
 * Requires: p ≡ 1 (mod 4) AND 2p-1 is a perfect square.
 * From check_emirp_gmp.c:31-40
 */
static bool is_consec_sq_sum(mpz_t p, mpz_t disc) {
    if (mpz_fdiv_ui(p, 4) != 1) return false;

    mpz_mul_ui(disc, p, 2);
    mpz_sub_ui(disc, disc, 1);

    return mpz_perfect_square_p(disc) != 0;
}

/* ── Reverse a decimal string in-place into caller's buffer ──────────
 * From check_emirp_gmp.c:43-52 (adapted: caller provides buffer)
 */
static void reverse_string(const char *str, int len, char *out) {
    for (int i = 0; i < len; i++)
        out[i] = str[len - 1 - i];
    out[len] = '\0';
}

/* ── Check if a decimal string is a palindrome ───────────────────────
 * From check_emirp_gmp.c:55-65
 */
static bool is_palindrome_str(const char *str, int len) {
    for (int i = 0; i < len / 2; i++) {
        if (str[i] != str[len - 1 - i])
            return false;
    }
    return true;
}

/* ── Solve n² + (n+1)² = p for n ─────────────────────────────────────
 * n = (-1 + sqrt(2p - 1)) / 2; returns true if n is a non-negative integer.
 * From check_converse_gmp.c:24-66
 */
static bool solve_consecutive_squares(mpz_t p, mpz_t n_out, mpz_t disc) {
    /* disc = 2p - 1 */
    mpz_mul_ui(disc, p, 2);
    mpz_sub_ui(disc, disc, 1);

    if (!mpz_perfect_square_p(disc))
        return false;

    /* sqrt_d = sqrt(disc) — reuse disc as temp after check */
    mpz_t sqrt_d;
    mpz_init(sqrt_d);
    mpz_sqrt(sqrt_d, disc);

    /* numerator = sqrt_d - 1 */
    mpz_sub_ui(sqrt_d, sqrt_d, 1);

    if (!mpz_divisible_ui_p(sqrt_d, 2)) {
        mpz_clear(sqrt_d);
        return false;
    }

    mpz_fdiv_q_ui(n_out, sqrt_d, 2);
    mpz_clear(sqrt_d);
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <max_prime>\n", argv[0]);
        fprintf(stderr, "       %s <start_n> <max_prime>\n", argv[0]);
        return 1;
    }

    /* ── Parse arguments ───────────────────────────────────────────── */
    uint64_t start_n = 0;
    mpz_t max_prime_z, max_n_z, temp;
    mpz_init(max_prime_z);
    mpz_init(max_n_z);
    mpz_init(temp);

    if (argc == 3) {
        /* Range mode: <start_n> <max_prime> */
        mpz_t start_n_z;
        mpz_init(start_n_z);
        if (mpz_set_str(start_n_z, argv[1], 10) != 0) {
            fprintf(stderr, "Error: Invalid start_n format\n");
            mpz_clear(start_n_z);
            mpz_clear(max_prime_z); mpz_clear(max_n_z); mpz_clear(temp);
            return 1;
        }
        start_n = mpz_get_ui(start_n_z);
        mpz_clear(start_n_z);

        if (mpz_set_str(max_prime_z, argv[2], 10) != 0) {
            fprintf(stderr, "Error: Invalid max_prime format\n");
            mpz_clear(max_prime_z); mpz_clear(max_n_z); mpz_clear(temp);
            return 1;
        }
    } else {
        /* Full mode: <max_prime> */
        if (mpz_set_str(max_prime_z, argv[1], 10) != 0) {
            fprintf(stderr, "Error: Invalid max_prime format\n");
            mpz_clear(max_prime_z); mpz_clear(max_n_z); mpz_clear(temp);
            return 1;
        }
    }

    /* max_n = sqrt(max_prime / 2) */
    mpz_fdiv_q_ui(temp, max_prime_z, 2);
    mpz_sqrt(max_n_z, temp);
    uint64_t max_n = mpz_get_ui(max_n_z);

    /* ── Banner ────────────────────────────────────────────────────── */
    printf("\n");
    printf("  CVPipe — Merged Con-verse Prime Pipeline\n");
    printf("==============================================================\n");
    gmp_printf("  Max prime:   %Zd\n", max_prime_z);
    printf("  Start N:     %lu\n", start_n);
    printf("  Max N:       %lu\n", max_n);
    printf("  Threads:     %d\n", NUM_THREADS);
    printf("  MR rounds:   %d\n", MR_ROUNDS);
    printf("  Formula:     2n^2 + 2n + 1\n");
    printf("  Output:      converse.dat, otto_primes.dat\n");
    printf("==============================================================\n\n");

    mpz_clear(max_prime_z);
    mpz_clear(max_n_z);
    mpz_clear(temp);

    /* ── Open output files ─────────────────────────────────────────── */
    FILE *fp_converse = fopen("converse.dat", "w");
    if (!fp_converse) {
        fprintf(stderr, "Error: cannot create converse.dat\n");
        return 1;
    }
    FILE *fp_otto = fopen("otto_primes.dat", "w");
    if (!fp_otto) {
        fprintf(stderr, "Error: cannot create otto_primes.dat\n");
        fclose(fp_converse);
        return 1;
    }

    time_t wall_start = time(NULL);
    double omp_start = omp_get_wtime();

    omp_set_num_threads(NUM_THREADS);

    /* ── Global statistics (reduced across threads) ────────────────── */
    uint64_t g_candidates = 0, g_gatekeeper = 0, g_palindromes = 0;
    uint64_t g_consec_sq  = 0, g_mr_tests   = 0, g_primes      = 0;
    uint64_t g_emirps     = 0, g_converse   = 0, g_otto        = 0;

    #pragma omp parallel reduction(+:g_candidates,g_gatekeeper,g_palindromes, \
                                     g_consec_sq,g_mr_tests,g_primes, \
                                     g_emirps,g_converse,g_otto)
    {
        int tid = omp_get_thread_num();
        thread_stats_t st = {0};

        /* Per-thread GMP variables */
        mpz_t n_z, candidate, reversed_num, disc, n1_out, n2_out;
        mpz_init(n_z);
        mpz_init(candidate);
        mpz_init(reversed_num);
        mpz_init(disc);
        mpz_init(n1_out);
        mpz_init(n2_out);

        /* Stack buffers — no malloc per iteration */
        char cand_str[256];
        char rev_str[256];

        /* Compute first n for this thread >= start_n */
        uint64_t first_n;
        if (start_n == 0) {
            first_n = (uint64_t)tid;
        } else {
            first_n = start_n;
            uint64_t rem = first_n % NUM_THREADS;
            if (rem != (uint64_t)tid) {
                first_n += ((uint64_t)tid - rem + NUM_THREADS) % NUM_THREADS;
            }
        }

        for (uint64_t n = first_n; n <= max_n; n += NUM_THREADS) {

            /* ── Step 1: Compute p = 2n² + 2n + 1 ─────────────────── */
            mpz_set_ui(n_z, n);
            mpz_mul(candidate, n_z, n_z);           /* n² */
            mpz_mul_ui(candidate, candidate, 2);     /* 2n² */
            mpz_addmul_ui(candidate, n_z, 2);        /* + 2n */
            mpz_add_ui(candidate, candidate, 1);      /* + 1 */
            st.candidates_generated++;

            /* ── Step 2: Convert to string (stack buffer) ──────────── */
            gmp_sprintf(cand_str, "%Zd", candidate);
            int len = strlen(cand_str);

            /* ── Step 3: Gatekeeper filter ─────────────────────────── */
            if (!is_valid_candidate(cand_str, len))
                continue;
            st.gatekeeper_passed++;

            /* ── Step 4: Palindrome branch ─────────────────────────── */
            if (is_palindrome_str(cand_str, len)) {
                st.palindromes_found++;
                /* MR test on palindromic candidate */
                st.mr_tests++;
                if (mpz_probab_prime_p(candidate, MR_ROUNDS) > 0) {
                    st.primes_found++;
                    /* Solve for n to format output */
                    if (solve_consecutive_squares(candidate, n1_out, disc)) {
                        st.otto_found++;
                        #pragma omp critical(otto_write)
                        {
                            gmp_fprintf(fp_otto, "%Zd %Zd\n", candidate, n1_out);
                            fflush(fp_otto);
                        }
                    }
                }
                continue;  /* palindromes are never emirps */
            }

            /* ── Step 5: Reverse digits ────────────────────────────── */
            reverse_string(cand_str, len, rev_str);

            /* ── Step 6: is_consec_sq_sum on reversed? ─────────────── */
            mpz_set_str(reversed_num, rev_str, 10);
            if (!is_consec_sq_sum(reversed_num, disc))
                continue;
            st.consec_sq_passed++;

            /* ── Step 7: MR on p ───────────────────────────────────── */
            st.mr_tests++;
            if (mpz_probab_prime_p(candidate, MR_ROUNDS) <= 0)
                continue;
            st.primes_found++;

            /* ── Step 8: MR on r ───────────────────────────────────── */
            st.mr_tests++;
            if (mpz_probab_prime_p(reversed_num, MR_ROUNDS) <= 0)
                continue;
            st.emirps_found++;

            /* ── Step 9: Solve consecutive squares for both ────────── */
            bool p_ok = solve_consecutive_squares(candidate, n1_out, disc);
            bool r_ok = solve_consecutive_squares(reversed_num, n2_out, disc);

            if (p_ok && r_ok) {
                /* Dedup: only write if p <= r */
                if (mpz_cmp(candidate, reversed_num) <= 0) {
                    st.converse_found++;
                    #pragma omp critical(converse_write)
                    {
                        gmp_fprintf(fp_converse, "%Zd %Zd %Zd %Zd\n",
                                    candidate, reversed_num, n1_out, n2_out);
                        fflush(fp_converse);
                        gmp_printf("\n  *** CON-VERSE PAIR FOUND ***\n");
                        gmp_printf("  %Zd <==> %Zd\n", candidate, reversed_num);
                        gmp_printf("  %Zd = %Zd^2 + (%Zd+1)^2\n",
                                   candidate, n1_out, n1_out);
                        gmp_printf("  %Zd = %Zd^2 + (%Zd+1)^2\n\n",
                                   reversed_num, n2_out, n2_out);
                    }
                }
            }

            /* ── Progress reporting (thread 0 only) ────────────────── */
            if (tid == 0 && st.candidates_generated % PROGRESS_INTERVAL == 0) {
                double elapsed = omp_get_wtime() - omp_start;
                double pct = 100.0 * (double)(n - start_n) /
                             (double)(max_n - start_n + 1);
                printf("  [%6.1fs] n=%lu  (%.1f%%)  gate=%lu  consec_sq=%lu  mr=%lu\n",
                       elapsed, n, pct,
                       st.gatekeeper_passed, st.consec_sq_passed, st.mr_tests);
                fflush(stdout);
            }
        }

        /* Reduce per-thread stats into globals */
        g_candidates  += st.candidates_generated;
        g_gatekeeper  += st.gatekeeper_passed;
        g_palindromes += st.palindromes_found;
        g_consec_sq   += st.consec_sq_passed;
        g_mr_tests    += st.mr_tests;
        g_primes      += st.primes_found;
        g_emirps      += st.emirps_found;
        g_converse    += st.converse_found;
        g_otto        += st.otto_found;

        mpz_clear(n_z);
        mpz_clear(candidate);
        mpz_clear(reversed_num);
        mpz_clear(disc);
        mpz_clear(n1_out);
        mpz_clear(n2_out);
    }

    fclose(fp_converse);
    fclose(fp_otto);

    double elapsed = omp_get_wtime() - omp_start;
    time_t wall_end = time(NULL);

    /* ── Summary ───────────────────────────────────────────────────── */
    printf("\n==============================================================\n");
    printf("  CVPipe COMPLETE\n");
    printf("==============================================================\n");
    printf("  Candidates generated:  %12lu\n", g_candidates);
    printf("  Gatekeeper passed:     %12lu\n", g_gatekeeper);
    printf("  Palindromes found:     %12lu\n", g_palindromes);
    printf("  Consec-sq pre-filter:  %12lu  (sent to MR)\n", g_consec_sq);
    printf("  Miller-Rabin tests:    %12lu\n", g_mr_tests);
    printf("  Primes confirmed:      %12lu\n", g_primes);
    printf("  Emirps found:          %12lu\n", g_emirps);
    printf("  CON-VERSE pairs:       %12lu\n", g_converse);
    printf("  Otto primes:           %12lu\n", g_otto);
    printf("  Wall time:             %12.1f seconds\n", difftime(wall_end, wall_start));
    printf("  CPU time:              %12.1f seconds\n", elapsed);
    printf("==============================================================\n");

    if (g_converse == 1) {
        printf("\n  EXACTLY ONE CON-VERSE PAIR FOUND!\n");
        printf("  This may be the only con-verse prime pair!\n");
        printf("==============================================================\n");
    } else if (g_converse == 0) {
        printf("\n  No con-verse pairs found in this range.\n");
        printf("==============================================================\n");
    } else {
        printf("\n  MULTIPLE CON-VERSE PAIRS FOUND!\n");
        printf("  This is a significant discovery!\n");
        printf("==============================================================\n");
    }

    return 0;
}
