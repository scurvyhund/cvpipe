#!/bin/bash
#
# CVPipe Distributed Search - nitroIII
# Range: 10^24 to 10^25 (66% of work)
# Machine: nitroIII (Ryzen 7 4700U, 64GB RAM, 16 threads)
# Expected runtime: ~4-5 hours
#

set -e  # Exit on error

echo "=========================================="
echo "CVPipe Distributed Search - nitroIII"
echo "=========================================="
echo "Range: 10^24 to 6.94×10^24 (66% of total)"
echo "Machine: nitroIII (16 threads)"
echo "Started: $(date)"
echo "=========================================="
echo ""

# Define ranges
START_N=707106781186
END_CANDIDATE=6940000000000001182793728

# Create output directory
OUTDIR="10e24-25-nitroIII-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$OUTDIR"
cd "$OUTDIR"

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
echo "nitroIII COMPLETE!"
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
echo "Ready to merge with nitroII results!"
