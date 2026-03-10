/* str_palindrome.c
 *
 * Palindrome detection for decimal strings
 */

#include "strarith.h"

bool str_is_palindrome(const char *str, int len) {
    for (int i = 0; i < len / 2; i++) {
        if (str[i] != str[len - 1 - i]) {
            return false;
        }
    }
    return true;
}
