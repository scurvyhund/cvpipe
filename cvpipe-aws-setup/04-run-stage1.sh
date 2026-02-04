#!/bin/bash
# 04-run-stage1.sh - Run CVPipe Stage 1 on nitroIII

set -e
source ./helper-functions.sh

echo "========================================"
echo "CVPipe Stage 1 - Local Execution"
echo "========================================"
echo ""

# Check storage is mounted
if [ ! -d "/mnt/cvp-working" ]; then
    echo "ERROR: /mnt/cvp-working not mounted"
    echo "Run: sudo ./01-storage-setup.sh first"
    exit 1
fi

# Find CVPipe installation
CVPIPE_DIR=""
for dir in ~/cvpipe ~/CVPipe ~/projects/cvpipe; do
    if [ -f "$dir/cvpipe" ]; then
        CVPIPE_DIR="$dir"
        break
    fi
done

if [ -z "$CVPIPE_DIR" ]; then
    echo "ERROR: Could not find CVPipe installation"
    echo "Please specify the path:"
    read -p "CVPipe directory: " CVPIPE_DIR
    if [ ! -f "$CVPIPE_DIR/cvpipe" ]; then
        echo "ERROR: cvpipe binary not found in $CVPIPE_DIR"
        exit 1
    fi
fi

echo "Using CVPipe from: $CVPIPE_DIR"
cd $CVPIPE_DIR

# Check if Stage 1 already completed
OUTPUT="/mnt/cvp-working/candidates-10e24.dat"
if [ -f "$OUTPUT" ]; then
    echo ""
    echo "WARNING: Stage 1 output already exists:"
    ls -lh $OUTPUT
    read -p "Re-run Stage 1? (y/n): " rerun
    if [ "$rerun" != "y" ]; then
        echo "Using existing candidates file"
        echo "Next step: ./05-deploy-to-cloud.sh"
        exit 0
    fi
    mv $OUTPUT ${OUTPUT}.backup.$(date +%Y%m%d-%H%M%S)
fi

echo ""
echo "=== Starting Stage 1 for 10^24 ==="
echo "This will take approximately 48-72 hours"
echo "Output: $OUTPUT"
echo ""
read -p "Press Enter to start..."

# Log start time
echo "Stage 1 started at $(date)" | tee /mnt/cvp-working/stage1.log

# Run Stage 1 with progress monitoring
./cvpipe --stage1 1e24 2>&1 | tee -a /mnt/cvp-working/stage1.log > $OUTPUT

# Log completion
echo "Stage 1 completed at $(date)" | tee -a /mnt/cvp-working/stage1.log

echo ""
echo "========================================"
echo "Stage 1 Complete!"
echo "========================================"
echo ""
ls -lh $OUTPUT
echo ""
CANDIDATES=$(wc -l < $OUTPUT)
echo "Candidates generated: $CANDIDATES"
echo ""
echo "Next step: Run ./05-deploy-to-cloud.sh"
