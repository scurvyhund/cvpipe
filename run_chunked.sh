#!/bin/bash
# run_chunked.sh - Chunked CVPipe orchestration for large decades
#
# Divides the n-range into chunks that fit on the NVMe SSD (~535 GB each),
# runs stages 1-3.5 per chunk, accumulates emirps/ottos on the HDD,
# then runs stage 4 once on the merged results.
#
# Usage: ./run_chunked.sh <start_prime> <end_prime> <num_chunks>
# Example: ./run_chunked.sh 10000000000000000000000000 100000000000000000000000000 16
#
# No C source changes required — all stages truncate output files on open.

set -euo pipefail

# ── Configuration ──────────────────────────────────────────────────
WORKDIR=/home/jim/cvpipe-run
RESULTSDIR=/mnt/cvp-working/data/run_${3:-chunked}
MIN_FREE_GB=200

# ── Argument parsing ──────────────────────────────────────────────
if [ $# -ne 3 ]; then
    echo "Usage: $0 <start_prime> <end_prime> <num_chunks>"
    echo "Example: $0 10000000000000000000000000 100000000000000000000000000 16"
    exit 1
fi

START_PRIME="$1"
END_PRIME="$2"
NUM_CHUNKS="$3"

# ── Compute n-range boundaries using python3 (big-integer math) ───
read -r N_START N_END CHUNK_SIZE <<< "$(python3 -c "
import math
start_p = int('$START_PRIME')
end_p = int('$END_PRIME')
n_start = math.isqrt(start_p // 2)
n_end = math.isqrt(end_p // 2)
chunk_size = (n_end - n_start) // $NUM_CHUNKS
print(n_start, n_end, chunk_size)
")"

# ── Setup ─────────────────────────────────────────────────────────
mkdir -p "$RESULTSDIR"

# Use run label based on primes range for the results directory
LABEL="$(python3 -c "print(f'{int(\"$START_PRIME\"):.0e}_to_{int(\"$END_PRIME\"):.0e}')")"
RESULTSDIR="/mnt/cvp-working/data/run_${LABEL}"
mkdir -p "$RESULTSDIR"

PROGRESS_FILE="$RESULTSDIR/progress.log"
touch "$PROGRESS_FILE"

echo "=============================================================="
echo "CVPipe Chunked Pipeline"
echo "=============================================================="
echo "  Start prime:   $START_PRIME"
echo "  End prime:     $END_PRIME"
echo "  N range:       $N_START to $N_END"
echo "  Chunk size:    $CHUNK_SIZE n-values"
echo "  Num chunks:    $NUM_CHUNKS"
echo "  Work dir:      $WORKDIR (NVMe)"
echo "  Results dir:   $RESULTSDIR (HDD)"
echo "=============================================================="
echo ""

cd "$WORKDIR"

# Verify binaries exist
for bin in gen_candidates_range_gmp filter_primes_gmp check_emirp_gmp check_palindrome_gmp check_converse_gmp; do
    if [ ! -x "./$bin" ]; then
        echo "ERROR: ./$bin not found or not executable in $WORKDIR"
        exit 1
    fi
done

TOTAL_START=$(date +%s)

# ── Main chunk loop ───────────────────────────────────────────────
for (( i=1; i<=NUM_CHUNKS; i++ )); do

    # Check if this chunk was already completed (resume support)
    if grep -q "^CHUNK_${i}_DONE$" "$PROGRESS_FILE" 2>/dev/null; then
        echo "[Chunk $i/$NUM_CHUNKS] Already completed — skipping"
        continue
    fi

    # Compute chunk boundaries via python3
    read -r CHUNK_N_START CHUNK_MAX_PRIME <<< "$(python3 -c "
n_start = $N_START
chunk_size = $CHUNK_SIZE
i = $i
num_chunks = $NUM_CHUNKS
n_end_total = $N_END

chunk_n_start = n_start + (i - 1) * chunk_size
if i == num_chunks:
    chunk_n_end = n_end_total  # last chunk covers remainder
else:
    chunk_n_end = n_start + i * chunk_size

# max_prime = 2*n^2 + 2*n + 1 at chunk_n_end
chunk_max_prime = 2 * chunk_n_end * chunk_n_end + 2 * chunk_n_end + 1
print(chunk_n_start, chunk_max_prime)
")"

    # Check NVMe free space
    FREE_GB=$(df --output=avail "$WORKDIR" | tail -1 | awk '{printf "%.0f", $1/1048576}')
    if [ "$FREE_GB" -lt "$MIN_FREE_GB" ]; then
        echo "ERROR: Only ${FREE_GB} GB free on NVMe (need ${MIN_FREE_GB} GB minimum)"
        exit 1
    fi

    CHUNK_START=$(date +%s)
    echo "=============================================================="
    echo "[Chunk $i/$NUM_CHUNKS] n_start=$CHUNK_N_START  max_prime=$CHUNK_MAX_PRIME"
    echo "[Chunk $i/$NUM_CHUNKS] NVMe free: ${FREE_GB} GB"
    echo "=============================================================="

    # Stage 1: Generate candidates
    echo "[Chunk $i/$NUM_CHUNKS] Stage 1: Generating candidates..."
    ./gen_candidates_range_gmp "$CHUNK_N_START" "$CHUNK_MAX_PRIME"

    # Stage 2: Filter primes
    echo "[Chunk $i/$NUM_CHUNKS] Stage 2: Filtering primes..."
    ./filter_primes_gmp

    # Stage 3: Emirp detection
    echo "[Chunk $i/$NUM_CHUNKS] Stage 3: Checking emirps..."
    ./check_emirp_gmp

    # Stage 3.5: Palindrome detection
    echo "[Chunk $i/$NUM_CHUNKS] Stage 3.5: Checking palindromes..."
    ./check_palindrome_gmp

    # Append results to cumulative files on HDD
    cat emirps.dat >> "$RESULTSDIR/emirps_all.dat"
    cat otto_primes.dat >> "$RESULTSDIR/otto_all.dat"

    CHUNK_END=$(date +%s)
    CHUNK_ELAPSED=$(( CHUNK_END - CHUNK_START ))

    echo ""
    echo "[Chunk $i/$NUM_CHUNKS] Complete in ${CHUNK_ELAPSED}s"
    echo "CHUNK_${i}_DONE" >> "$PROGRESS_FILE"
    echo ""
done

# ── Stage 4: Converse check on merged emirps ──────────────────────
echo "=============================================================="
echo "Stage 4: Converse check on accumulated emirps"
echo "=============================================================="

cp "$RESULTSDIR/emirps_all.dat" emirps.dat
./check_converse_gmp

# Save final results to HDD
cp converse.dat "$RESULTSDIR/"

TOTAL_END=$(date +%s)
TOTAL_ELAPSED=$(( TOTAL_END - TOTAL_START ))

echo ""
echo "=============================================================="
echo "CHUNKED PIPELINE COMPLETE"
echo "  Total elapsed: ${TOTAL_ELAPSED}s ($(( TOTAL_ELAPSED / 3600 ))h $(( (TOTAL_ELAPSED % 3600) / 60 ))m)"
echo "  Results:       $RESULTSDIR/"
echo "=============================================================="
