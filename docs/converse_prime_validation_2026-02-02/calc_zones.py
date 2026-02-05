#!/usr/bin/env python3
"""
calc_zones.py

Calculate valid search zones based on proven digit constraints
for the formula 2n² + 2n + 1

Valid last-two digits: 01, 13, 21, 41, 61, 81 (proven by cycle analysis)
Valid first-two digits for emirps: 10, 12, 14, 16, 18, 31 (derived from symmetry)

Usage: ./calc_zones.py <min_prime> <max_prime>
Example: ./calc_zones.py 1000000000000000000000000 10000000000000000000000000
"""

import sys
import math
from gmpy2 import mpz, isqrt

# Valid first-digit patterns (proven)
VALID_PATTERNS = [10, 12, 14, 16, 18, 31]

def prime_to_n(prime):
    """Convert prime p to n using: n = floor((-1 + sqrt(2p - 1)) / 2)"""
    prime = mpz(prime)
    two_p_minus_1 = 2 * prime - 1
    sqrt_val = isqrt(two_p_minus_1)
    n = (sqrt_val - 1) // 2
    return int(n)

def count_digits(num):
    """Count decimal digits in a number"""
    return len(str(num))

def main():
    if len(sys.argv) != 3:
        print("Usage: ./calc_zones.py <min_prime> <max_prime>")
        print("Example: ./calc_zones.py 1000000000000000000000000 10000000000000000000000000")
        sys.exit(1)
    
    try:
        prime_min = mpz(sys.argv[1])
        prime_max = mpz(sys.argv[2])
    except:
        print("Error: Invalid prime values")
        sys.exit(1)
    
    print("=" * 80)
    print("Zone Calculator for 2n² + 2n + 1 Search")
    print("=" * 80)
    print(f"Prime range: {prime_min} to {prime_max}")
    print()
    
    # Determine digit range
    d_min = count_digits(prime_min)
    d_max = count_digits(prime_max)
    
    print(f"Digit range: {d_min} to {d_max} digits")
    print("Valid patterns: 10, 12, 14, 16, 18, 31 (proven from cycle analysis)")
    print()
    
    # Storage for zones
    zones = []
    
    print("=" * 80)
    print("VALID SEARCH ZONES:")
    print("=" * 80)
    
    # For each digit count
    for d in range(d_min, d_max + 1):
        
        # Calculate 10^(d-2) for this digit range
        power_of_10 = mpz(10) ** (d - 2)
        
        # For each valid pattern
        for pattern in VALID_PATTERNS:
            
            # Calculate prime range for this pattern
            # Pattern "12" at d=25 means primes from 12×10^23 to 13×10^23 - 1
            p_zone_min = pattern * power_of_10
            p_zone_max = (pattern + 1) * power_of_10 - 1
            
            # Intersect with requested range [prime_min, prime_max]
            p_min = max(p_zone_min, prime_min)
            p_max = min(p_zone_max, prime_max)
            
            # Skip if no overlap
            if p_min > p_max:
                continue
            
            # Convert prime bounds to n bounds
            n_min = prime_to_n(p_min)
            n_max = prime_to_n(p_max)
            
            # Store zone info
            zone = {
                'n_min': n_min,
                'n_max': n_max,
                'pattern': pattern,
                'digits': d,
                'prime_min': p_min,
                'prime_max': p_max
            }
            zones.append(zone)
            
            # Print zone info
            print(f"\nZone {len(zones)}:")
            print(f"  Pattern:      {pattern} (digits starting with {pattern})")
            print(f"  Prime range:  {float(p_min):.3e} to {float(p_max):.3e}")
            print(f"  n range:      {n_min:,} to {n_max:,}")
            print(f"  n values:     {n_max - n_min + 1:,}")
    
    print("\n")
    print("=" * 80)
    print("SUMMARY:")
    print("=" * 80)
    print(f"Total valid zones: {len(zones)}")
    
    # Calculate total n-values to search
    total_n = sum(z['n_max'] - z['n_min'] + 1 for z in zones)
    print(f"Total n-values:    {total_n:,}")
    
    # Calculate what would have been searched without zone-skip
    naive_n_min = prime_to_n(prime_min)
    naive_n_max = prime_to_n(prime_max)
    naive_total = naive_n_max - naive_n_min + 1
    
    print(f"\nWithout zone-skip: {naive_total:,} n-values")
    print(f"With zone-skip:    {total_n:,} n-values")
    reduction_pct = 100.0 * (naive_total - total_n) / naive_total
    speedup = naive_total / total_n
    print(f"Reduction:         {reduction_pct:.1f}% ({speedup:.2f}x speedup)")
    
    print("\n")
    print("=" * 80)
    print("ZONE-SKIP CODE TEMPLATE (C):")
    print("=" * 80)
    print()
    print("// Add this to your search program:")
    print("typedef struct { uint64_t n_min; uint64_t n_max; int pattern; } SearchZone;")
    print()
    print("SearchZone zones[] = {")
    for i, z in enumerate(zones):
        comma = "," if i < len(zones) - 1 else ""
        print(f"    {{ {z['n_min']}ULL, {z['n_max']}ULL, {z['pattern']} }}{comma}")
    print("};")
    print(f"int num_zones = {len(zones)};")
    print()
    print("// Search only valid zones:")
    print("for (int z = 0; z < num_zones; z++) {")
    print("    uint64_t n_start = zones[z].n_min;")
    print("    uint64_t n_end = zones[z].n_max;")
    print("    for (uint64_t n = n_start; n <= n_end; n++) {")
    print("        // Your candidate generation code here")
    print("    }")
    print("}")
    print()
    
    # Also output shell script for distributed runs
    print("=" * 80)
    print("DISTRIBUTED SEARCH SCRIPT:")
    print("=" * 80)
    print()
    print("# Split zones between machines for balanced workload")
    print()
    for i, z in enumerate(zones):
        print(f"# Zone {i+1}: pattern {z['pattern']}, {z['n_max'] - z['n_min'] + 1:,} n-values")
        print(f"./gen_candidates_range_gmp {z['n_min']} {z['prime_max']}")
        print()

if __name__ == "__main__":
    main()
