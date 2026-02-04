#!/bin/bash
# run_pipeline.sh - Run CVPipe with timing and logging

MAX_PRIME=${1:-20000000000000000000}  # Default: 2×10^19
LOGFILE="pipeline_$(date +%Y%m%d_%H%M%S).log"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  CVPipe Timed Run                                         ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Max prime:  $MAX_PRIME"
echo "║  Log file:   $LOGFILE"
echo "║  Start time: $(date)"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Record start time
START=$(date +%s)

# Run pipeline
./generate_candidates $MAX_PRIME 2>&1 | tee -a $LOGFILE
./filter_primes 2>&1 | tee -a $LOGFILE
./check_emirp 2>&1 | tee -a $LOGFILE
./check_palindrome 2>&1 | tee -a $LOGFILE
./check_converse 2>&1 | tee -a $LOGFILE

# Record end time
END=$(date +%s)
ELAPSED=$((END - START))

# Calculate hours, minutes, seconds
HOURS=$((ELAPSED / 3600))
MINUTES=$(((ELAPSED % 3600) / 60))
SECONDS=$((ELAPSED % 60))

echo "" | tee -a $LOGFILE
echo "╔════════════════════════════════════════════════════════════╗" | tee -a $LOGFILE
echo "║  PIPELINE COMPLETE                                        ║" | tee -a $LOGFILE
echo "╠════════════════════════════════════════════════════════════╣" | tee -a $LOGFILE
echo "║  End time:     $(date)                ║" | tee -a $LOGFILE
echo "║  Elapsed:      ${HOURS}h ${MINUTES}m ${SECONDS}s                                   ║" | tee -a $LOGFILE
echo "║  Log saved:    $LOGFILE                   ║" | tee -a $LOGFILE
echo "╚════════════════════════════════════════════════════════════╝" | tee -a $LOGFILE

echo ""
echo "Results:"
echo "  Otto primes:  $(wc -l < otto_primes.dat 2>/dev/null || echo 0) found"
echo "  Con-verse:    $(wc -l < converse.dat 2>/dev/null || echo 0) found"
