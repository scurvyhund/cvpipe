#!/bin/bash
# 07-download-results.sh - Download results from S3

set -e
source ./helper-functions.sh

echo "========================================"
echo "Download CVPipe Results"
echo "========================================"
echo ""

check_file ~/.cvpipe-bucket "Run ./03-aws-bootstrap.sh first"
BUCKET=$(cat ~/.cvpipe-bucket)

echo "S3 Bucket: $BUCKET"

# Check completion status
COMPLETED=$(aws s3 ls s3://$BUCKET/completed/ 2>/dev/null | wc -l)
echo "Completed workers: $COMPLETED / 20"

if [ $COMPLETED -lt 20 ]; then
    echo ""
    echo "WARNING: Not all workers have completed"
    read -p "Download results anyway? (y/n): " proceed
    if [ "$proceed" != "y" ]; then
        echo "Aborting. Run ./06-monitor-progress.sh to check status"
        exit 0
    fi
fi

# Check storage
if [ ! -d "/mnt/cvp-archive/10e24" ]; then
    echo "Creating /mnt/cvp-archive/10e24/"
    mkdir -p /mnt/cvp-archive/10e24/{results,logs}
fi

echo ""
echo "=== Downloading results from S3 ==="
aws s3 sync s3://$BUCKET/results/ /mnt/cvp-archive/10e24/results/ --quiet
RESULT_COUNT=$(ls -1 /mnt/cvp-archive/10e24/results/ 2>/dev/null | wc -l)
echo "Results downloaded: $RESULT_COUNT files"

echo ""
echo "=== Downloading logs from S3 ==="
aws s3 sync s3://$BUCKET/logs/ /mnt/cvp-archive/10e24/logs/ --quiet
LOG_COUNT=$(ls -1 /mnt/cvp-archive/10e24/logs/ 2>/dev/null | wc -l)
echo "Logs downloaded: $LOG_COUNT files"

echo ""
echo "=== Merging results ==="
if [ $RESULT_COUNT -gt 0 ]; then
    cat /mnt/cvp-archive/10e24/results/result-*.dat > /mnt/cvp-archive/10e24/final-results.dat 2>/dev/null || true
    
    if [ -f /mnt/cvp-archive/10e24/final-results.dat ]; then
        HITS=$(wc -l < /mnt/cvp-archive/10e24/final-results.dat)
        echo "Total hits found: $HITS"
        echo "Merged file: /mnt/cvp-archive/10e24/final-results.dat"
    else
        echo "No results to merge (this is expected if no converse primes found)"
    fi
else
    echo "No result files to merge"
fi

echo ""
echo "=== Disk usage ==="
du -sh /mnt/cvp-archive/10e24/

echo ""
echo "========================================"
echo "Download Complete!"
echo "========================================"
echo ""
echo "Results location: /mnt/cvp-archive/10e24/"
echo ""
ls -lh /mnt/cvp-archive/10e24/
echo ""
echo "Next step: Run ./08-cleanup.sh to delete S3 data and stop charges"
