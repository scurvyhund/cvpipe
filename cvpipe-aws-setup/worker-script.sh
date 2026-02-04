#!/bin/bash
# worker-script.sh - Runs on each EC2 instance
# This script is uploaded to instances via user-data

set -e

# Configuration (replaced by deployment script)
BUCKET="YOUR_BUCKET_NAME"

# Get instance metadata
INSTANCE_ID=$(ec2-metadata --instance-id | cut -d" " -f2)
INSTANCE_TYPE=$(ec2-metadata --instance-type | cut -d" " -f2)

# Log function
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a /tmp/cvpipe-worker.log
}

log "CVPipe Worker starting"
log "Instance: $INSTANCE_ID ($INSTANCE_TYPE)"
log "Bucket: $BUCKET"

# Update system
log "Updating system packages..."
yum update -y >> /tmp/cvpipe-worker.log 2>&1

# Install dependencies
log "Installing build dependencies..."
yum install -y gcc gmp-devel make git bc >> /tmp/cvpipe-worker.log 2>&1

# Setup working directory
cd /home/ec2-user
log "Working directory: $(pwd)"

# Download CVPipe source
log "Downloading CVPipe source..."
aws s3 cp s3://$BUCKET/cvpipe.tar.gz . >> /tmp/cvpipe-worker.log 2>&1
tar xzf cvpipe.tar.gz
cd cvpipe || { log "ERROR: cvpipe directory not found"; exit 1; }

# Compile CVPipe
log "Compiling CVPipe..."
make clean >> /tmp/cvpipe-worker.log 2>&1
make -j$(nproc) >> /tmp/cvpipe-worker.log 2>&1

if [ ! -f ./cvpipe ]; then
    log "ERROR: cvpipe binary not created"
    exit 1
fi

log "CVPipe compiled successfully"

# Download candidates
log "Downloading candidates..."
aws s3 cp s3://$BUCKET/candidates.dat . >> /tmp/cvpipe-worker.log 2>&1

CANDIDATE_COUNT=$(wc -l < candidates.dat)
log "Candidates loaded: $CANDIDATE_COUNT"

# Run Stage 2
log "Running Stage 2..."
START_TIME=$(date +%s)
./cvpipe --stage2 candidates.dat > stage2.out 2> stage2.log
STAGE2_TIME=$(($(date +%s) - START_TIME))
STAGE2_COUNT=$(wc -l < stage2.out 2>/dev/null || echo 0)
log "Stage 2 complete: $STAGE2_COUNT candidates, ${STAGE2_TIME}s"

# Run Stage 3 if Stage 2 produced output
if [ $STAGE2_COUNT -gt 0 ]; then
    log "Running Stage 3..."
    START_TIME=$(date +%s)
    ./cvpipe --stage3 stage2.out > stage3.out 2> stage3.log
    STAGE3_TIME=$(($(date +%s) - START_TIME))
    STAGE3_COUNT=$(wc -l < stage3.out 2>/dev/null || echo 0)
    log "Stage 3 complete: $STAGE3_COUNT candidates, ${STAGE3_TIME}s"
    
    # Run Stage 4 if Stage 3 produced output
    if [ $STAGE3_COUNT -gt 0 ]; then
        log "Running Stage 4..."
        START_TIME=$(date +%s)
        ./cvpipe --stage4 stage3.out > results.dat 2> stage4.log
        STAGE4_TIME=$(($(date +%s) - START_TIME))
        RESULT_COUNT=$(wc -l < results.dat 2>/dev/null || echo 0)
        log "Stage 4 complete: $RESULT_COUNT results, ${STAGE4_TIME}s"
        
        # Upload results
        if [ $RESULT_COUNT -gt 0 ]; then
            log "Uploading results..."
            aws s3 cp results.dat s3://$BUCKET/results/result-$INSTANCE_ID.dat
            log "Results uploaded"
        else
            log "No results found (expected for this range)"
        fi
    else
        log "Stage 3 produced no output, skipping Stage 4"
    fi
else
    log "Stage 2 produced no output, skipping Stages 3 and 4"
fi

# Upload logs
log "Uploading logs..."
cp /tmp/cvpipe-worker.log ./worker.log
aws s3 cp worker.log s3://$BUCKET/logs/worker-$INSTANCE_ID.log
aws s3 cp stage2.log s3://$BUCKET/logs/stage2-$INSTANCE_ID.log 2>/dev/null || true
aws s3 cp stage3.log s3://$BUCKET/logs/stage3-$INSTANCE_ID.log 2>/dev/null || true
aws s3 cp stage4.log s3://$BUCKET/logs/stage4-$INSTANCE_ID.log 2>/dev/null || true

# Signal completion
log "Signaling completion..."
echo "Completed at $(date)" > completion.txt
aws s3 cp completion.txt s3://$BUCKET/completed/$INSTANCE_ID

log "Worker complete, shutting down..."

# Shutdown instance
shutdown -h +1
