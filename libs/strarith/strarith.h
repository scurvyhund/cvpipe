/* strarith.h
 *
 * Pure C String Arithmetic Library
 * No external dependencies - works with char* and basic C types
 *
 * Functions for digit manipulation, validation, and string operations
 * on decimal number representations.
 */

#ifndef STRARITH_H
#define STRARITH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned __int128   u128;

/* ────────────────────────────────────────────────────────────────────
 * String Reversal
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Reverse a string into caller-provided buffer
 * @param str Input string
 * @param len Length of string
 * @param out Output buffer (must be at least len+1 bytes)
 */
void str_reverse(const char *str, int len, char *out);

/**
 * Reverse a string using thread-local buffer (thread-safe)
 * @param str Input string
 * @return Pointer to thread-local reversed string (max 255 chars)
 * @note Return value is only valid until next call in same thread
 */
char* str_reverse_tls(const char *str);


/* ────────────────────────────────────────────────────────────────────
 * Palindrome Detection
 * ──────────────────────────────────────────────────────────────────── *

**
 * Check if a string is a palindrome
 * @param str Input string
 * @param len Length of string
 * @return true if palindrome, false otherwise
 *
 * bool str_is_palindrome(const char *str, int len);
 */

/* ────────────────────────────────────────────────────────────────────
 * Digit Pattern Validation
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Check if candidate matches valid first/last 2-digit patterns
 * for consecutive square emirps (2n² + 2n + 1)
 *
 * Valid first2: {10, 12, 14, 16, 18, 31}
 * Valid last2: ends in 1 or 3, and ≡ 1 (mod 4)
 *
 * @param str Input string (decimal number)
 * @param len Length of string
 * @return true if valid candidate, false otherwise
 *
 * bool str_is_valid_candidate(const char *str, int len);
 *
**
 * Extract first n digits from a decimal string
 * @param str Input string
 * @param n Number of digits to extract
 * @return Integer value of first n digits, or -1 on error
 *
 * int str_first_n_digits(const char *str, int n);
 *
 *
 * Extract last n digits from a decimal string
 * @param str Input string
 * @param n Number of digits to extract
 * @return Integer value of last n digits, or -1 on error
 *
 * int str_last_n_digits(const char *str, int n);
 */

/* ────────────────────────────────────────────────────────────────────
 * Digit Counting
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Count decimal digits in a string (excluding leading/trailing whitespace)
 * @param str Input string
 * @return Number of digits
 */
int str_count_digits(const char *str);

/* Compare two numeric strings numerically (not lexicographically)
* Returns: < 0 if a < b, 0 if a == b, > 0 if a > b
 */
__attribute__((warn_unused_result))
int string_compare(const char* a, const char* b);

/* String multiplication using the big-mul.c algorithm
 * This is the core function adapted from my big-mul.c
 */
__attribute__((malloc))  __attribute__((warn_unused_result))
char* string_multiply(const char* num1_str, const char* num2_str);

/* String addition */
__attribute__((malloc)) __attribute__((warn_unused_result)) 
char* string_add(const char* a, const char* b);

/* String subtraction (assumes a >= b) */
__attribute__((malloc)) __attribute__((warn_unused_result))
char* string_subtract(const char* a, const char* b);

/* Divide string by 2 */
__attribute__((malloc)) __attribute__((warn_unused_result))
char* string_divide_by_2(const char* num_str);

/* Helper: Binary search to find input / divisor */
__attribute__((malloc)) __attribute__((warn_unused_result))
char* string_divide_by_power(const char* input_str, const char* divisor);

/* Helper: Binary search to find input / divisor */
__attribute__((malloc)) __attribute__((warn_unused_result))
char* string_mod_power_of_2_128(const char* input_str);

__attribute__((malloc)) __attribute__((warn_unused_result))
char* string_mod_power_of_2_256(const char* input_str);

/* Calculate input_str % (2^128) using string arithmetic */
__attribute__((malloc)) __attribute__((warn_unused_result))
char* calc_wrap_cnt_string(const char* input_str);

// Conv str to u128 (only works if str reps value < 2^128) 
u128 string_to_u128(const char* str);

#endif /* STRARITH_H */
