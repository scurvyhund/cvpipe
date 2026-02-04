#!/bin/bash
# backup_run.sh - Archive CVPipe results with metadata
# 
# Usage: ./backup_run.sh [run_name]
# Example: ./backup_run.sh 10e21_search

RUN_NAME=${1:-"run_$(date +%Y%m%d_%H%M%S)"}
BACKUP_DIR="backups"
ARCHIVE_FILE="${BACKUP_DIR}/${RUN_NAME}.tar.gz"

# Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

echo "======================================================================="
echo "CVPipe Backup Script"
echo "======================================================================="
echo "Run name: $RUN_NAME"
echo ""

# Check what files exist
echo "Checking for files to backup..."

FILES_TO_BACKUP=""
TOTAL_SIZE=0

# Check candidates
if ls p*.dat 1> /dev/null 2>&1; then
    CANDIDATE_SIZE=$(du -sb p*.dat | awk '{sum+=$1} END {print sum}')
    TOTAL_SIZE=$((TOTAL_SIZE + CANDIDATE_SIZE))
    echo "  ✓ Candidates (p*.dat): $(numfmt --to=iec $CANDIDATE_SIZE)"
    FILES_TO_BACKUP="$FILES_TO_BACKUP p*.dat"
fi

# Check primes
if ls primes*.dat 1> /dev/null 2>&1; then
    PRIMES_SIZE=$(du -sb primes*.dat | awk '{sum+=$1} END {print sum}')
    TOTAL_SIZE=$((TOTAL_SIZE + PRIMES_SIZE))
    PRIMES_COUNT=$(cat primes*.dat | wc -l)
    echo "  ✓ Primes (primes*.dat): $(numfmt --to=iec $PRIMES_SIZE) ($PRIMES_COUNT primes)"
    FILES_TO_BACKUP="$FILES_TO_BACKUP primes*.dat"
fi

# Check emirps
if [ -f emirps.dat ]; then
    EMIRPS_SIZE=$(stat -f%z emirps.dat 2>/dev/null || stat -c%s emirps.dat)
    TOTAL_SIZE=$((TOTAL_SIZE + EMIRPS_SIZE))
    EMIRPS_COUNT=$(wc -l < emirps.dat)
    echo "  ✓ Emirps: $(numfmt --to=iec $EMIRPS_SIZE) ($EMIRPS_COUNT pairs)"
    FILES_TO_BACKUP="$FILES_TO_BACKUP emirps.dat"
fi

# Check otto primes
if [ -f otto_primes.dat ]; then
    OTTO_SIZE=$(stat -f%z otto_primes.dat 2>/dev/null || stat -c%s otto_primes.dat)
    TOTAL_SIZE=$((TOTAL_SIZE + OTTO_SIZE))
    OTTO_COUNT=$(wc -l < otto_primes.dat)
    echo "  ✓ Otto primes: $(numfmt --to=iec $OTTO_SIZE) ($OTTO_COUNT found)"
    FILES_TO_BACKUP="$FILES_TO_BACKUP otto_primes.dat"
fi

# Check con-verse
if [ -f converse.dat ]; then
    CONVERSE_SIZE=$(stat -f%z converse.dat 2>/dev/null || stat -c%s converse.dat)
    TOTAL_SIZE=$((TOTAL_SIZE + CONVERSE_SIZE))
    CONVERSE_COUNT=$(wc -l < converse.dat)
    echo "  ✓ Con-verse pairs: $(numfmt --to=iec $CONVERSE_SIZE) ($CONVERSE_COUNT found)"
    FILES_TO_BACKUP="$FILES_TO_BACKUP converse.dat"
fi

if [ -z "$FILES_TO_BACKUP" ]; then
    echo ""
    echo "✗ No files found to backup!"
    echo "  Make sure you're in the directory with .dat files"
    exit 1
fi

echo ""
echo "Total size: $(numfmt --to=iec $TOTAL_SIZE)"
echo ""

# Create metadata file
METADATA_FILE=".run_metadata_${RUN_NAME}.txt"
cat > "$METADATA_FILE" << EOF
CVPipe Run Metadata
======================================================================
Run name:        $RUN_NAME
Backup date:     $(date)
Hostname:        $(hostname)
User:            $(whoami)

Files backed up:
----------------------------------------------------------------------
$(ls -lh $FILES_TO_BACKUP)

Summary:
----------------------------------------------------------------------
Total candidates: $(ls p*.dat 2>/dev/null | wc -l) files
Total primes:     ${PRIMES_COUNT:-0}
Emirp pairs:      ${EMIRPS_COUNT:-0}
Otto primes:      ${OTTO_COUNT:-0}
Con-verse pairs:  ${CONVERSE_COUNT:-0}

System info:
----------------------------------------------------------------------
CPU: $(lscpu | grep "Model name" | cut -d: -f2 | xargs)
Cores: $(nproc)
RAM: $(free -h | grep Mem | awk '{print $2}')

GMP version: $(gcc --version | head -1)

Notes:
----------------------------------------------------------------------
(Add any notes here)
EOF

FILES_TO_BACKUP="$FILES_TO_BACKUP $METADATA_FILE"

# Create the archive
echo "Creating compressed archive..."
echo ""

tar -czf "$ARCHIVE_FILE" $FILES_TO_BACKUP

if [ $? -eq 0 ]; then
    ARCHIVE_SIZE=$(stat -f%z "$ARCHIVE_FILE" 2>/dev/null || stat -c%s "$ARCHIVE_FILE")
    COMPRESSION_RATIO=$(echo "scale=1; $TOTAL_SIZE / $ARCHIVE_SIZE" | bc)
    
    echo "======================================================================="
    echo "✓ BACKUP COMPLETE"
    echo "======================================================================="
    echo "Archive:          $ARCHIVE_FILE"
    echo "Size:             $(numfmt --to=iec $ARCHIVE_SIZE)"
    echo "Original size:    $(numfmt --to=iec $TOTAL_SIZE)"
    echo "Compression:      ${COMPRESSION_RATIO}x"
    echo ""
    echo "To extract later:"
    echo "  tar -xzf $ARCHIVE_FILE"
    echo ""
    echo "To list contents:"
    echo "  tar -tzf $ARCHIVE_FILE"
    echo "======================================================================="
    
    # Clean up metadata file
    rm "$METADATA_FILE"
else
    echo "✗ Backup failed!"
    rm -f "$METADATA_FILE"
    exit 1
fi
