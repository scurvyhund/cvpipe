/* test_strarith.c
 *
 * Test program for libstrarith.a (Pure C String Library)
 *
 * Compile: gcc -O2 test_strarith.c -L../strarith -lstrarith -o test_strarith
 * Run: ./test_strarith
 */

#include <stdio.h>
#include <string.h>
#include "../strarith/strarith.h"

int main(void) {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  libstrarith.a Test Suite (Pure C String Library)           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* ── Test 1: String Reversal ─────────────────────────────────── */
    printf("Test 1: String Reversal\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *test1 = "12641";
    char rev1[256];
    str_reverse(test1, strlen(test1), rev1);
    printf("  Input:    %s\n", test1);
    printf("  Reversed: %s\n", rev1);
    printf("  Expected: 14621\n");
    printf("  Result:   %s\n\n", strcmp(rev1, "14621") == 0 ? "✓ PASS" : "✗ FAIL");

    /* ── Test 2: Thread-local Reversal ───────────────────────────── */
    printf("Test 2: Thread-local String Reversal\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *test2 = "3187813";
    char *rev2 = str_reverse_tls(test2);
    printf("  Input:    %s\n", test2);
    printf("  Reversed: %s\n", rev2);
    printf("  Expected: 3187813 (palindrome)\n");
    printf("  Result:   %s\n\n", strcmp(rev2, "3187813") == 0 ? "✓ PASS" : "✗ FAIL");

    /* ── Test 3: Palindrome Detection ────────────────────────────── */
    printf("Test 3: Palindrome Detection\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *pal1 = "12321";
    const char *pal2 = "12345";
    printf("  %s is palindrome: %s (expected: yes)\n", pal1,
           str_is_palindrome(pal1, strlen(pal1)) ? "yes" : "no");
    printf("  %s is palindrome: %s (expected: no)\n", pal2,
           str_is_palindrome(pal2, strlen(pal2)) ? "yes" : "no");
    printf("  Result:   %s\n\n",
           (str_is_palindrome(pal1, strlen(pal1)) &&
            !str_is_palindrome(pal2, strlen(pal2))) ? "✓ PASS" : "✗ FAIL");

    /* ── Test 4: Candidate Validation ────────────────────────────── */
    printf("Test 4: Candidate Validation (emirp digit patterns)\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *valid1   = "12641";   // starts with 12, ends with 41
    const char *valid2   = "10000001"; // starts with 10, ends with 01
    const char *invalid1 = "22641";   // starts with 22 (invalid)
    const char *invalid2 = "12642";   // ends with 42 (not ≡ 1 mod 4)

    printf("  %s: %s (expected: valid)\n", valid1,
           str_is_valid_candidate(valid1, strlen(valid1)) ? "valid" : "invalid");
    printf("  %s: %s (expected: valid)\n", valid2,
           str_is_valid_candidate(valid2, strlen(valid2)) ? "valid" : "invalid");
    printf("  %s: %s (expected: invalid)\n", invalid1,
           str_is_valid_candidate(invalid1, strlen(invalid1)) ? "valid" : "invalid");
    printf("  %s: %s (expected: invalid)\n", invalid2,
           str_is_valid_candidate(invalid2, strlen(invalid2)) ? "valid" : "invalid");

    bool test4_pass = str_is_valid_candidate(valid1, strlen(valid1)) &&
                      str_is_valid_candidate(valid2, strlen(valid2)) &&
                      !str_is_valid_candidate(invalid1, strlen(invalid1)) &&
                      !str_is_valid_candidate(invalid2, strlen(invalid2));
    printf("  Result:   %s\n\n", test4_pass ? "✓ PASS" : "✗ FAIL");

    /* ── Test 5: Digit Extraction ────────────────────────────────── */
    printf("Test 5: Digit Extraction\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *num = "12641";
    int first2 = str_first_n_digits(num, 2);
    int last2  = str_last_n_digits(num, 2);
    printf("  Number:   %s\n", num);
    printf("  First 2:  %d (expected: 12)\n", first2);
    printf("  Last 2:   %d (expected: 41)\n", last2);
    printf("  Result:   %s\n\n", (first2 == 12 && last2 == 41) ? "✓ PASS" : "✗ FAIL");

    /* ── Test 6: Digit Counting ──────────────────────────────────── */
    printf("Test 6: Digit Counting\n");
    printf("───────────────────────────────────────────────────────────────\n");
    const char *num1 = "12641";
    const char *num2 = "100000000000000000000000001";  // 27 digits
    printf("  %s has %d digits (expected: 5)\n", num1, str_count_digits(num1));
    printf("  %s... has %d digits (expected: 27)\n", "10...01", str_count_digits(num2));
    printf("  Result:   %s\n\n",
           (str_count_digits(num1) == 5 && str_count_digits(num2) == 27) ?
           "✓ PASS" : "✗ FAIL");

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  All tests complete!                                         ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    return 0;
}
