# String and GMP Arithmetic Libraries

Two modular static libraries for consecutive square prime research:

## Libraries

###  **libstrarith.a** - Pure C String Arithmetic
**No external dependencies** - works with `char*` and basic C types

**Functions:**
- `str_reverse()` - Reverse decimal strings
- `str_reverse_tls()` - Thread-safe reversal with thread-local buffer
- `str_is_palindrome()` - Palindrome detection
- `str_is_valid_candidate()` - Validate digit patterns for emirps
- `str_first_n_digits()` / `str_last_n_digits()` - Digit extraction
- `str_count_digits()` - Count decimal digits

###  **libgmparith.a** - GMP Arithmetic Library
**Requires:** GMP library (`-lgmp`)

**Functions:**
- `gmp_count_digits()` - Count digits in GMP integers
- `gmp_prime_to_n()` - Convert prime → n (inverse formula)
- `gmp_n_to_prime()` - Convert n → prime (forward formula)
- `gmp_is_consec_sq_sum()` - Quick test: is p = n² + (n+1)²?
- `gmp_solve_consec_sqr()` - Solve for n given p
- `gmp_verify_consecutive_squares()` - Full verification with recompute
- `gmp_compute_zones()` - Zone-skip optimization for searches

## Build

```bash
# Build both libraries
make

# Build individually
make strarith
make gmparith

# Build and run tests
make test
cd test && make run

# Clean all
make clean
```

## Usage Examples

### Pure String Library

```c
#include "strarith.h"

// Reverse a number string
char rev[256];
str_reverse("12641", 5, rev);  // rev = "14621"

// Check palindrome
bool is_pal = str_is_palindrome("3187813", 7);  // true

// Validate emirp candidate
bool valid = str_is_valid_candidate("12641", 5);  // true (starts with 12, ends with 41)
```

**Compile:**
```bash
gcc -O3 your_program.c -L/path/to/libs/strarith -lstrarith -o your_program
```

### GMP Library

```c
#include <gmp.h>
#include "gmparith.h"

mpz_t p, n, disc;
mpz_init(p);
mpz_init(n);
mpz_init(disc);

// Test if 12641 is consecutive square sum
mpz_set_ui(p, 12641);
if (gmp_is_consec_sq_sum(p, disc)) {
    // Solve for n
    gmp_solve_consec_sqr(p, n, disc);  // n = 79
    gmp_printf("%Zd² + %Zd² = %Zd\n", n, n, p);
}
```

**Compile:**
```bash
gcc -O3 your_program.c -L/path/to/libs/gmparith -lgmparith -lgmp -o your_program
```

## Library Contents

View contents:
```bash
make list
```

View exported symbols:
```bash
make symbols
```

## File Structure

```
libs/
├── Makefile                  # Master build file
├── README.md                 # This file
├── strarith/
│   ├── strarith.h           # Public API
│   ├── str_reverse.c
│   ├── str_palindrome.c
│   ├── str_validate.c
│   ├── libstrarith.a        # Built library
│   └── Makefile
├── gmparith/
│   ├── gmparith.h           # Public API
│   ├── gmp_digits.c
│   ├── gmp_conversion.c
│   ├── gmp_consecutive.c
│   ├── gmp_zones.c
│   ├── libgmparith.a        # Built library
│   └── Makefile
└── test/
    ├── test_strarith.c      # Pure string tests
    ├── test_gmparith.c      # GMP tests
    └── Makefile
```

## Adding New Functions

Static libraries are trivial to extend:

```bash
# 1. Write your new function in a .c file
vim strarith/str_newfunction.c

# 2. Add to header
vim strarith/strarith.h

# 3. Add object to Makefile OBJS
vim strarith/Makefile

# 4. Rebuild
make strarith
```

The archive (`ar`) tool automatically adds/updates:
```bash
ar r libstrarith.a str_newfunction.o
```

## Design Philosophy

**Separation of Concerns:**
- Pure string functions have zero dependencies
- GMP functions isolated from string manipulation
- Clean API boundaries
- Easy to test independently

**Benefits:**
-  Reusable across projects
-  No `-lgmp` needed for pure string operations
-  Incremental growth - add functions as needed
-  Easy to audit and optimize individual modules

## References

These libraries extract and modularize functions from:
- `cvpipe.c` - Merged converse prime pipeline
- `utils/calc_zones_gmp.c` - Zone calculation
- `four_stage_cvpipe/*.c` - Four-stage pipeline components

Optimized for consecutive square prime search (2n² + 2n + 1).
