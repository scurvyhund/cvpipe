/* str_validate.c
 *
 * Digit pattern validation for consecutive square numbers
 */

#include "strarith.h"
#include <string.h>
#include <ctype.h>

bool str_is_valid_candidate(const char *str, int len) {
    if (len < 2) return false;

    // Extract first two digits
    int first_digit  = str[0] - '0';
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
    int last_digit   = str[len - 1] - '0';
    int second_last  = str[len - 2] - '0';
    int last2 = second_last * 10 + last_digit;

    return (last_digit == 1 || last_digit == 3) && (last2 % 4 == 1);
}

int str_first_n_digits(const char *str, int n) {
    if (!str || n <= 0) return -1;

    int len = strlen(str);
    if (n > len) n = len;

    int result = 0;
    for (int i = 0; i < n && isdigit(str[i]); i++) {
        result = result * 10 + (str[i] - '0');
    }

    return result;
}

int str_last_n_digits(const char *str, int n) {
    if (!str || n <= 0) return -1;

    int len = strlen(str);
    if (n > len) n = len;

    int result = 0;
    for (int i = len - n; i < len && isdigit(str[i]); i++) {
        result = result * 10 + (str[i] - '0');
    }

    return result;
}

int str_count_digits(const char *str) {
    if (!str) return 0;

    int count = 0;
    while (*str) {
        if (isdigit(*str)) {
            count++;
        }
        str++;
    }

    return count;
}
