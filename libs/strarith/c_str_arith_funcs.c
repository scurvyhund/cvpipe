/* This C file contains function definitions for the libstrarith.a static library.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdalign.h>

typedef     unsigned __int128   u128;

// The magic constant: 2^128 in decimal string form
const char* POW_2_128 = "340282366920938463463374607431768211456";
// 2^256 in decimal (77 digits)
const char* POW_2_256 = "115792089237316195423570985008687907853269984665640564039457584007913129639936";



/* Compare two numeric strings numerically (not lexicographically)
 * Returns: < 0 if a < b, 0 if a == b, > 0 if a > b
 */
int string_compare(const char* a, const char* b) {
    // Skip leading zeros
    while (*a == '0' && *(a + 1) != '\0') a++;
    while (*b == '0' && *(b + 1) != '\0') b++;

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
 
    // Different lengths = different magnitudes
  if (len_a != len_b) {
    return (len_a < len_b) ? -1 : 1;
  }

  // Same length, compare digit by digit
  return strcmp(a, b);
}

/* String multiplication using the big-mul.c algorithm
 * This is the core function adapted from my big-mul.c
 */
char* string_multiply(const char* num1_str, const char* num2_str) {
    // Handle zero cases
    if (strcmp(num1_str, "0") == 0 || strcmp(num2_str, "0") == 0) {
        char *result = malloc(2);
        strcpy(result, "0");
        return result;
    }
 
    // Handle one cases
    if (strcmp(num1_str, "1") == 0) {
        char *result = malloc(strlen(num2_str) + 1);
        strcpy(result, num2_str);
        return result;
    }

    if (strcmp(num2_str, "1") == 0) {
        char *result = malloc(strlen(num1_str) + 1);
        strcpy(result, num1_str);
        return result;
    }
 
    int len_1 = strlen(num1_str);
    int len_2 = strlen(num2_str);
 
    // Convert strings to digit arrays (like my conv_cmdlnArg function)
    int array_1[len_1];
    int array_2[len_2];
 
    // Convert num1_str to integer array
    for(int i = 0; i < len_1; i++) {
        array_1[i] = num1_str[i] - '0';
    }
 
    // Convert num2_str to integer array
    for(int i = 0; i < len_2; i++) {
        array_2[i] = num2_str[i] - '0';
    }
 
    // Create product array (adapted from the array_mult function)
    int product_len = len_1 + len_2;
    int product_array[product_len];
 
    // Initialize product array to zeros
    for(int i = 0; i < product_len; i++) {
        product_array[i] = 0;
    }
 
    // Multiplication algorithm from big-mul.c
    int carry = 0;
    int tmp;
    int dec_k_indx = 0;
    // Set i to index 1's digit in multiplier (array_1)
    int i = len_1 - 1;

    for(; i >= 0; i--) {
        int k = product_len - 1 - dec_k_indx++;
        int j = len_2 - 1;
 
        // Iterate over each digit in multiplicand (array_2)
        while(j >= 0 || carry > 0) {
          if(j >= 0)
            tmp = array_1[i] * array_2[j];
          else 
            tmp = 0;
 
          tmp += carry;
          carry = tmp / 10;
 
          product_array[k] += (tmp % 10);
          carry += (product_array[k] / 10);
          product_array[k] = product_array[k] % 10;
 
          j--;
          k--;
        }
    }
 
    // Convert back to string for output...
    char *result = malloc(product_len + 1);
    int start = 0;
 
    // Remove leading zeros
    while(start < product_len && product_array[start] == 0) {
        start++;
    }

    // Handle case where result is zero
    if(start == product_len) {
        strcpy(result, "0");
        return result;
    }
 
    // Convert digits back to string
    for(int i = start; i < product_len; i++) {
        result[i - start] = product_array[i] + '0';
    }
    result[product_len - start] = '\0';
 
    return result;
}

/* String addition */
char* string_add(const char* a, const char* b) {
    int len_a = strlen(a);
    int len_b = strlen(b);
    int max_len = (len_a > len_b ? len_a : len_b) + 2;
 
    char *result = calloc(max_len, 1);
    int carry = 0;
    int pos = 0;
 
    int i = len_a - 1;
    int j = len_b - 1;
 
    while (i >= 0 || j >= 0 || carry) {
      int digit_a = (i >= 0) ? (a[i] - '0') : 0;
      int digit_b = (j >= 0) ? (b[j] - '0') : 0;
 
      int sum = digit_a + digit_b + carry;
      carry = sum / 10;
      result[pos++] = (sum % 10) + '0';
 
      i--;
      j--;
    }
 
    // Reverse result
    for (int k = 0; k < pos / 2; k++) {
      char temp = result[k];
      result[k] = result[pos - 1 - k];
      result[pos - 1 - k] = temp;
    }
 
    result[pos] = '\0';
    return result;
}

/* String subtraction (assumes a >= b) */
char* string_subtract(const char* a, const char* b) {
    int len_a = strlen(a);
    int len_b = strlen(b);
 
    char *result = calloc(len_a + 1, 1);
    int borrow = 0;
    int pos = 0;

    int i = len_a - 1;
    int j = len_b - 1;
 
    while (i >= 0 || j >= 0) {
      int digit_a = (i >= 0) ? (a[i] - '0') : 0;
      int digit_b = (j >= 0) ? (b[j] - '0') : 0;
      int diff = digit_a - digit_b - borrow;
 
      if (diff < 0) {
         diff += 10;
         borrow = 1;
      } else {
         borrow = 0;
      }
 
      result[pos++] = diff + '0';
      i--;
      j--;
    }
 
    // Reverse result
    for (int k = 0; k < pos / 2; k++) {
      char temp = result[k];
      result[k] = result[pos - 1 - k];
      result[pos - 1 - k] = temp;
    }

    result[pos] = '\0';
 
    // Remove leading zeros
    char *start = result;
    while (*start == '0' && *(start + 1) != '\0') {
       start++;
    }
 
    if (start != result) {
        memmove(result, start, strlen(start) + 1);
    }
 
    return result;
}

/* Divide string by 2 */
char* string_divide_by_2(const char* num_str) {
    int len = strlen(num_str);
    char *result = calloc(len + 1, 1);
 
    int remainder = 0;
    int pos = 0;
 
    for (int i = 0; i < len; i++) {
      int current = remainder * 10 + (num_str[i] - '0');
      result[pos++] = (current / 2) + '0';
      remainder = current % 2;
    }

    result[pos] = '\0';
 
    // Remove leading zeros
    char *start = result;
    while (*start == '0' && *(start + 1) != '\0') {
      start++;
    }
 
    if (start != result) {
      memmove(result, start, strlen(start) + 1);
    }
 
    return result;
}

/* Helper: Binary search to find input / divisor */
char* string_divide_by_power(const char* input_str, const char* divisor) {
    // If input < divisor, return "0"
    if (string_compare(input_str, divisor) < 0) {
        char *result = malloc(2);
        strcpy(result, "0");
        return result;
    }
    
    // Binary search
    char *lo = malloc(2);
    strcpy(lo, "1");
    
    char *hi = malloc(2);
    strcpy(hi, "1");
    
    // Find upper bound
    while (1) {
        char *test_val = string_multiply(divisor, hi);
        int cmp = string_compare(test_val, input_str);
        free(test_val);
        
        if (cmp > 0) break;
        
        char *new_hi = string_multiply(hi, "2");
        free(hi);
        hi = new_hi;
        
        if (strlen(hi) > 100) {
            free(lo);
            free(hi);
            return NULL;
        }
    }
    
    char *result = malloc(2);
    strcpy(result, "0");
    
    // Binary search for exact quotient
    while (string_compare(lo, hi) <= 0) {
        char *diff = string_subtract(hi, lo);
        char *half_diff = string_divide_by_2(diff);
        char *mid = string_add(lo, half_diff);
        
        char *mid_multiple = string_multiply(divisor, mid);
        int cmp = string_compare(mid_multiple, input_str);
        
        if (cmp <= 0) {
            free(result);
            result = malloc(strlen(mid) + 1);
            strcpy(result, mid);
            
            char *new_lo = string_add(mid, "1");
            free(lo);
            lo = new_lo;
        } else {
            char *new_hi = string_subtract(mid, "1");
            free(hi);
            hi = new_hi;
        }
        
        free(diff);
        free(half_diff);
        free(mid);
        free(mid_multiple);
    }
    
    free(lo);
    free(hi);
    return result;
}

/* Calculate input_str % (2^128) using string arithmetic */
char* string_mod_power_of_2_128(const char* input_str) {
    // If input < 2^128, return copy of input
    if (string_compare(input_str, POW_2_128) < 0) {
        char *result = malloc(strlen(input_str) + 1);
        strcpy(result, input_str);
        return result;
    }
    
    // quotient = input / 2^128
    char *quotient = string_divide_by_power(input_str, POW_2_128);
    if (!quotient) return NULL;
    
    // multiple = quotient * 2^128
    char *multiple = string_multiply(POW_2_128, quotient);
    
    // remainder = input - multiple
    char *remainder = string_subtract(input_str, multiple);
    
    free(quotient);
    free(multiple);
    return remainder;
}

/* Calculate input_str % (2^256) using string arithmetic */
char* string_mod_power_of_2_256(const char* input_str) {
    // If input < 2^256, return copy of input
    if (string_compare(input_str, POW_2_256) < 0) {
        char *result = malloc(strlen(input_str) + 1);
        strcpy(result, input_str);
        return result;
    }
    
    // quotient = input / 2^256
    char *quotient = string_divide_by_power(input_str, POW_2_256);
    if (!quotient) return NULL;
    
    // multiple = quotient * 2^256
    char *multiple = string_multiply(POW_2_256, quotient);
    
    // remainder = input - multiple
    char *remainder = string_subtract(input_str, multiple);
    
    free(quotient);
    free(multiple);
    return remainder;
}

// Calculate how many times input wraps around 2^256 using string arithmetic 
char* calc_wrap_cnt_string(const char* input_str) {
    // If input < 2^256, no wraparound
    if (string_compare(input_str, POW_2_256) < 0) {
        char *result = malloc(2);
        strcpy(result, "0");
        return result;
    }
    
    // Binary search using string arithmetic
    char *lo = malloc(2);
    strcpy(lo, "1");
    
    // Find upper bound by doubling
    char *hi = malloc(2);
    strcpy(hi, "1");
    
    while (1) {
        char *test_val = string_multiply(POW_2_256, hi);
        int cmp = string_compare(test_val, input_str);
        free(test_val);
        
        if (cmp > 0) break;  // Found upper bound
        
        char *new_hi = string_multiply(hi, "2");
        free(hi);
        hi = new_hi;
        
        // Sanity check to prevent infinite loops
        if (strlen(hi) > 100) {
            fprintf(stderr, "Error: Number too large for processing\n");
            free(lo);
            free(hi);
            return NULL;
        }
    }
    
    char *result = malloc(2);
    strcpy(result, "0");
    
    // Binary search
    while (string_compare(lo, hi) <= 0) {
        // Calculate mid = lo + (hi - lo) / 2
        char *diff = string_subtract(hi, lo);
        char *half_diff = string_divide_by_2(diff);
        char *mid = string_add(lo, half_diff);
        
        char *mid_multiple = string_multiply(POW_2_256, mid);
        int cmp = string_compare(mid_multiple, input_str);
        
        if (cmp <= 0) {
            free(result);
            result = malloc(strlen(mid) + 1);
            strcpy(result, mid);  // This multiplier works
            
            char *new_lo = string_add(mid, "1");
            free(lo);
            lo = new_lo;
        } else {
            char *new_hi = string_subtract(mid, "1");
            free(hi);
            hi = new_hi;
        }
        
        free(diff);
        free(half_diff);
        free(mid);
        free(mid_multiple);
    }
    
    free(lo);
    free(hi);
    return result;
}

// Conv str factor (cmdln arg) to u128 (only works if str reps value < 2^128) 
u128 string_to_u128(const char* str) {
    u128 result = 0;
    for(int i = 0; str[i]; i++) {
        result = result * 10 + (str[i] - '0');
    }
    return result;
}

void conv2base(u128 dec_val, uint8_t shft) {
    alignas(16) u128 base = 1;
    size_t cnt = 1;

    while(base < dec_val) {
        base <<= shft;
        cnt++;

        if(shft == 4 && cnt == 32)
            break;

        if (cnt == 128)
            break; 
    }

    uint8_t zeros;
    if(shft == 4) 
        zeros = 32-cnt;
    else 
        zeros = 128-cnt;

    size_t total_digits = zeros + cnt;
    size_t digit_count = 0;

    // Print leading zeros with proper spacing
    for(size_t i = 0; i < zeros; i++) {
        printf("0");
        digit_count++;
        
        if(digit_count % 4 == 0 && digit_count < total_digits)
            printf(" ");
    }

    while(base >= 1) {
        uint8_t quot = dec_val / base ;
        printf("%x", quot);
        digit_count++;

        if(digit_count % 4 == 0 && digit_count < total_digits)
            printf(" ");

        dec_val -= (base * quot);
        base >>= shft;
    }
}

