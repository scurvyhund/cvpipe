#!/bin/bash
#
# CVPipe Distributed Search - Merge Results
# Combines results from nitroIII and nitroII
#

set -e

if [ $# -ne 2 ]; then
   echo "Usage: $0 <nitroIII_dir> <nitroII_dir>"
   echo "Example: $0 10e24-25-nitroIII-20260201-120000 10e24-25-nitroII-20260201-120000"
   exit 1
fi

NITRO3_DIR=$1
NITRO2_DIR=$2

echo "=========================================="
echo "CVPipe Distributed Search - Merge Results"
echo "=========================================="
echo "nitroIII directory: $NITRO3_DIR"
echo "nitroII directory:  $NITRO2_DIR"
echo ""

# Verify directories exist
if [ ! -d "$NITRO3_DIR" ]; then
   echo "ERROR: nitroIII directory not found: $NITRO3_DIR"
   exit 1
fi

if [ ! -d "$NITRO2_DIR" ]; then
   echo "ERROR: nitroII directory not found: $NITRO2_DIR"
   exit 1
fi

# Create merged output directory
MERGED_DIR="10e24-25-merged-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$MERGED_DIR"

echo "Creating merged directory: $MERGED_DIR"
echo ""

# Merge candidates (p*.dat files)
echo "Merging candidate files..."
cat "$NITRO3_DIR"/p*.dat > "$MERGED_DIR/all_candidates.dat" 2>/dev/null || true
cat "$NITRO2_DIR"/p*.dat >> "$MERGED_DIR/all_candidates.dat" 2>/dev/null || true
CAND_COUNT=$(wc -l < "$MERGED_DIR/all_candidates.dat")
echo "  Total candidates: $CAND_COUNT"

# Merge primes
echo "Merging prime files..."
cat "$NITRO3_DIR"/primes*.dat > "$MERGED_DIR/all_primes.dat" 2>/dev/null || true
cat "$NITRO2_DIR"/primes*.dat >> "$MERGED_DIR/all_primes.dat" 2>/dev/null || true
PRIME_COUNT=$(wc -l < "$MERGED_DIR/all_primes.dat")
echo "  Total primes: $PRIME_COUNT"

# Merge emirps
echo "Merging emirp files..."
cat "$NITRO3_DIR/emirps.dat" "$NITRO2_DIR/emirps.dat" > "$MERGED_DIR/emirps.dat" 2>/dev/null || true
EMIRP_COUNT=$(wc -l < "$MERGED_DIR/emirps.dat")
echo "  Total emirps: $EMIRP_COUNT"

# Merge Otto primes
echo "Merging Otto prime files..."
cat "$NITRO3_DIR/otto_primes.dat" "$NITRO2_DIR/otto_primes.dat" > "$MERGED_DIR/otto_primes.dat" 2>/dev/null || true
OTTO_COUNT=$(wc -l < "$MERGED_DIR/otto_primes.dat")
echo "  Total Otto primes: $OTTO_COUNT"

# Merge converse results
echo "Merging converse results..."
cat "$NITRO3_DIR/converse.dat" "$NITRO2_DIR/converse.dat" > "$MERGED_DIR/converse.dat" 2>/dev/null || true
CONV_COUNT=$(wc -l < "$MERGED_DIR/converse.dat")
echo "  Total converse pairs: $CONV_COUNT"

# Create summary
echo ""
echo "=========================================="
echo "MERGE COMPLETE!"
echo "=========================================="
echo ""
echo "Combined Results:"
echo "  Candidates:      $CAND_COUNT"
echo "  Primes:          $PRIME_COUNT"
echo "  Emirps:          $EMIRP_COUNT"
echo "  Otto primes:     $OTTO_COUNT"
echo "  Converse pairs:  $CONV_COUNT"
echo ""
echo "Merged files in: $MERGED_DIR"
echo ""

# File sizes
echo "File sizes:"
du -h "$MERGED_DIR"/* | sort -h

# Create metadata file
cat > "$MERGED_DIR/search_metadata.txt" << EOF
CVPipe Distributed Search - 10^24 to 10^25
==========================================

Search Range:
  Start: 10^24
  End:   10^25

Machine Distribution:
  nitroIII (66%): 10^24 to 6.94×10^24
  nitroII  (33%): 6.94×10^24 to 10^25

Results:
  Candidates:      $CAND_COUNT
  Primes:          $PRIME_COUNT
  Emirps:          $EMIRP_COUNT
  Otto primes:     $OTTO_COUNT
  Converse pairs:  $CONV_COUNT

Merged: $(date)
nitroIII source: $NITRO3_DIR
nitroII source:  $NITRO2_DIR
EOF

echo ""
echo "Metadata saved to: $MERGED_DIR/search_metadata.txt"
echo ""
echo "All done! 🎉"
