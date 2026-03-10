/* str_reverse.c
 *
 * String reversal functions for decimal number strings
 */

#include <string.h>
#include <stdlib.h>
#include "strarith.h"

void str_reverse(const char *str, int len, char *out) {
    for (int i = 0; i < len; i++) {
        out[i] = str[len - 1 - i];
    }
    out[len] = '\0';
}

char* str_reverse_tls(const char *str) {
    static __thread char reversed[256];  // Thread-safe, per-thread buffer
    int len = strlen(str);

    if (len >= 256) {
        reversed[0] = '\0';
        return reversed;  // Error: string too long
    }

    for (int i = 0; i < len; i++) {
        reversed[i] = str[len - 1 - i];
    }
    reversed[len] = '\0';
    return reversed;
}
