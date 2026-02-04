#!/bin/bash
# 06-monitor-progress.sh - Monitor CVPipe cloud processing

source ./helper-functions.sh

check_file ~/.cvpipe-bucket "Run ./03-aws-bootstrap.sh first"
BUCKET=$(cat ~/.cvpipe-bucket)

# Check for cost-only mode
if [ "$1" == "--cost" ]; then
    echo "=== Current AWS Costs ==="
    START_DATE=$(date -d "7 days ago" +%Y-%m-%d)
    END_DATE=$(date -d "tomorrow" +%Y-%m-%d)
    
    aws ce get-cost-and-usage \
        --time-period Start=$START_DATE,End=$END_DATE \
        --granularity DAILY \
        --metrics BlendedCost \
        --query 'ResultsByTime[*].[TimePeriod.Start,Total.BlendedCost.Amount]' \
        --output table
    exit 0
fi

echo "========================================"
echo "CVPipe Cloud Processing Monitor"
echo "========================================"
echo ""
echo "Press Ctrl+C to exit"
echo ""

while true; do
    clear
    echo "========================================"
    echo "CVPipe Status - $(date)"
    echo "========================================"
    echo ""
    
    # Count completed instances
    COMPLETED=$(aws s3 ls s3://$BUCKET/completed/ 2>/dev/null | wc -l)
    TOTAL=20
    PERCENT=$((COMPLETED * 100 / TOTAL))
    
    echo "Progress: $COMPLETED / $TOTAL instances ($PERCENT%)"
    
    # Progress bar
    BARS=$((PERCENT / 5))
    printf "["
    for i in $(seq 1 $BARS); do printf "="; done
    for i in $(seq $BARS 19); do printf " "; done
    printf "] $PERCENT%%\n"
    
    echo ""
    
    # Instance status
    echo "=== Running Instances ==="
    RUNNING=$(aws ec2 describe-instances \
        --filters "Name=tag:Project,Values=10e24" "Name=instance-state-name,Values=running" \
        --query 'Reservations[*].Instances[*].[InstanceId,InstanceType,LaunchTime]' \
        --output text 2>/dev/null | wc -l)
    echo "Active workers: $RUNNING"
    
    if [ $RUNNING -gt 0 ]; then
        aws ec2 describe-instances \
            --filters "Name=tag:Project,Values=10e24" "Name=instance-state-name,Values=running" \
            --query 'Reservations[*].Instances[*].[InstanceId,InstanceType,LaunchTime]' \
            --output table 2>/dev/null || true
    fi
    
    echo ""
    
    # Check for results
    RESULTS=$(aws s3 ls s3://$BUCKET/results/ 2>/dev/null | wc -l)
    echo "Results uploaded: $RESULTS"
    
    LOGS=$(aws s3 ls s3://$BUCKET/logs/ 2>/dev/null | wc -l)
    echo "Log files: $LOGS"
    
    echo ""
    
    # Estimated cost
    echo "=== Estimated Cost ==="
    # c7a.8xlarge spot ~ $0.30/hr, 20 instances
    # Rough estimate based on running time
    if [ -f ~/.cvpipe-launch-time ]; then
        LAUNCH_TIME=$(cat ~/.cvpipe-launch-time)
        NOW=$(date +%s)
        HOURS=$(( (NOW - LAUNCH_TIME) / 3600 ))
        COST=$(echo "$HOURS * 20 * 0.30" | bc)
        echo "Runtime: ${HOURS}h"
        echo "Estimated: \$${COST}"
    else
        echo "Launch time not recorded"
        date +%s > ~/.cvpipe-launch-time
    fi
    
    echo ""
    
    # Check if complete
    if [ $COMPLETED -eq $TOTAL ]; then
        echo "========================================"
        echo "ALL WORKERS COMPLETE!"
        echo "========================================"
        echo ""
        echo "Run ./07-download-results.sh to collect results"
        exit 0
    fi
    
    # Check for failures
    FAILED=$(aws ec2 describe-instances \
        --filters "Name=tag:Project,Values=10e24" "Name=instance-state-name,Values=terminated,stopped" \
        --query 'Reservations[*].Instances[*].[InstanceId,StateReason.Message]' \
        --output text 2>/dev/null | wc -l)
    
    if [ $FAILED -gt 0 ]; then
        echo "WARNING: $FAILED instances failed/terminated"
    fi
    
    echo ""
    echo "Refreshing in 30 seconds... (Ctrl+C to exit)"
    sleep 30
done
