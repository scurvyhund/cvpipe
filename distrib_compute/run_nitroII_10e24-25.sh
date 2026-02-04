#!/bin/bash
#
# CVPipe Distributed Search - nitroII
# Range: 10^24 to 10^25 (33% of work)
# Machine: nitroII (i7-10510U, 40GB RAM, 8 threads)
# Expected runtime: ~4-5 hours
#

set -e  # Exit on error

echo "=========================================="
echo "CVPipe Distributed Search - nitroII"
echo "=========================================="
echo "Range: 6.94×10^24 to 10^25 (33% of total)"
echo "Machine: nitroII (8 threads)"
echo "Started: $(date)"
echo "=========================================="
echo ""

# Define ranges
START_N=1862793601019
END_CANDIDATE=10000000000000000000000000

# Create output directory
OUTDIR="10e24-25-nitroII-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUTDIR"
cd "$OUTDIR"

# Adjust thread count for nitroII (8 threads instead of 16)
export OMP_NUM_THREADS=8

# Stage 1: Generate candidates
echo "Stage 1: Generating candidates..."
START_TIME=$(date +%s)
time ../gen_candidates_range_gmp $START_N $END_CANDIDATE
STAGE1_TIME=$(($(date +%s) - START_TIME))
echo "Stage 1 complete: ${STAGE1_TIME}s"
echo ""

# Stage 2: Filter primes
echo "Stage 2: Filtering primes..."
START_TIME=$(date +%s)
time ../filter_primes_gmp
STAGE2_TIME=$(($(date +%s) - START_TIME))
echo "Stage 2 complete: ${STAGE2_TIME}s"
echo ""

# Stage 3: Check emirps
echo "Stage 3: Checking emirps..."
START_TIME=$(date +%s)
time ../check_emirp_gmp
STAGE3_TIME=$(($(date +%s) - START_TIME))
echo "Stage 3 complete: ${STAGE3_TIME}s"
echo ""

# Stage 3.5: Check palindromes
echo "Stage 3.5: Checking palindromes..."
START_TIME=$(date +%s)
time ../check_palindrome_gmp
STAGE35_TIME=$(($(date +%s) - START_TIME))
echo "Stage 3.5 complete: ${STAGE35_TIME}s"
echo ""

# Stage 4: Check converse
echo "Stage 4: Checking converse..."
START_TIME=$(date +%s)
time ../check_converse_gmp
STAGE4_TIME=$(($(date +%s) - START_TIME))
echo "Stage 4 complete: ${STAGE4_TIME}s"
echo ""

# Summary
echo "=========================================="
echo "nitroII COMPLETE!"
echo "=========================================="
echo "Finished: $(date)"
echo ""
echo "Timing breakdown:"
echo "  Stage 1 (candidates): ${STAGE1_TIME}s"
echo "  Stage 2 (primes):     ${STAGE2_TIME}s"
echo "  Stage 3 (emirps):     ${STAGE3_TIME}s"
echo "  Stage 3.5 (otto):     ${STAGE35_TIME}s"
echo "  Stage 4 (converse):   ${STAGE4_TIME}s"
TOTAL_TIME=$((STAGE1_TIME + STAGE2_TIME + STAGE3_TIME + STAGE35_TIME + STAGE4_TIME))
echo "  Total runtime:        ${TOTAL_TIME}s ($((TOTAL_TIME/3600))h $((TOTAL_TIME%3600/60))m)"
echo ""
echo "Results in: $OUTDIR"
echo ""
echo "Files created:"
ls -lh p*.dat primes*.dat emirps.dat otto_primes.dat converse.dat 2>/dev/null || true
echo ""
echo "Ready to merge with nitroIII results!"
