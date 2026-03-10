/* This is a program to test results of libstrarith.a static library
 * functions.
 *
 * Note, often when editing the source an ERROR is indicated. If it appears
 * that there is no reason for the error try compiling with:
 *
 * $> make clean && make
 *
 * If src compiles clean it's probably a bug in the nvim config caused by the
 * clang compiler being referenced even though we are using gcc...it may be 
 * due to the lsp for clang???
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "strarith.h"

int main(int argc, char** argv) {
    char* caller = string_multiply(argv[1], argv[2]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    caller = string_add(argv[1], argv[2]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    caller = string_subtract(argv[1], argv[2]);
    printf("\n%s\n", caller);
    free(caller);
    
    caller = NULL;

    caller = string_divide_by_2(argv[1]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    caller = string_divide_by_power(argv[1], argv[2]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    caller = string_mod_power_of_2_128(argv[1]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    caller = string_mod_power_of_2_256(argv[1]);
    printf("\n%s\n", caller);
    free(caller);
    
    caller = NULL;

    caller = calc_wrap_cnt_string(argv[1]);
    printf("\n%s\n", caller);
    free(caller);

    caller = NULL;

    return 0;
}
