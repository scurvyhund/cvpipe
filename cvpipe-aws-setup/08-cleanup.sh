#!/bin/bash
# 08-cleanup.sh - Clean up AWS resources

set -e
source ./helper-functions.sh

echo "========================================"
echo "AWS Resource Cleanup"
echo "========================================"
echo ""

check_file ~/.cvpipe-bucket "No bucket to clean up"
BUCKET=$(cat ~/.cvpipe-bucket)

echo "This will DELETE:"
echo "  - S3 bucket: $BUCKET (and all contents)"
echo "  - Launch template: CVPipeWorker-10e24"
echo "  - Any running test instances"
echo ""
echo "This will NOT delete:"
echo "  - IAM roles (CVPipeWorkerRole, CVPipeWorkerProfile)"
echo "  - Local results in /mnt/cvp-archive/"
echo ""
read -p "Continue? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
    echo "Cleanup cancelled"
    exit 0
fi

echo ""
echo "=== Checking for running instances ==="
RUNNING=$(aws ec2 describe-instances \
    --filters "Name=tag:Project,Values=10e24" "Name=instance-state-name,Values=running,pending" \
    --query 'Reservations[*].Instances[*].InstanceId' \
    --output text)

if [ -n "$RUNNING" ]; then
    echo "Found running instances:"
    echo $RUNNING
    read -p "Terminate these instances? (yes/no): " terminate
    if [ "$terminate" == "yes" ]; then
        aws ec2 terminate-instances --instance-ids $RUNNING
        echo "Instances terminating..."
    fi
fi

# Check for test instance
if [ -f ~/.cvpipe-test-instance ]; then
    TEST_INSTANCE=$(cat ~/.cvpipe-test-instance)
    STATE=$(aws ec2 describe-instances \
        --instance-ids $TEST_INSTANCE \
        --query 'Reservations[0].Instances[0].State.Name' \
        --output text 2>/dev/null || echo "not-found")
    
    if [ "$STATE" == "running" ]; then
        echo ""
        echo "Test instance $TEST_INSTANCE is still running"
        read -p "Terminate test instance? (y/n): " term_test
        if [ "$term_test" == "y" ]; then
            aws ec2 terminate-instances --instance-ids $TEST_INSTANCE
            echo "Test instance terminating..."
        fi
    fi
fi

echo ""
echo "=== Deleting S3 bucket ==="
echo "Bucket: $BUCKET"

# List contents
echo "Contents:"
aws s3 ls s3://$BUCKET/ --recursive --human-readable | head -20
TOTAL=$(aws s3 ls s3://$BUCKET/ --recursive | wc -l)
echo "... ($TOTAL total objects)"

read -p "Delete bucket and all contents? (yes/no): " delete_bucket
if [ "$delete_bucket" == "yes" ]; then
    aws s3 rb s3://$BUCKET --force
    echo "Bucket deleted"
    rm ~/.cvpipe-bucket
else
    echo "Bucket preserved"
fi

echo ""
echo "=== Deleting launch template ==="
aws ec2 delete-launch-template --launch-template-name CVPipeWorker-10e24 2>/dev/null && \
    echo "Launch template deleted" || echo "Launch template not found"

echo ""
echo "=== Final verification ==="
echo "Checking for any remaining 10e24 resources..."

# Check instances
REMAINING=$(aws ec2 describe-instances \
    --filters "Name=tag:Project,Values=10e24" \
    --query 'Reservations[*].Instances[*].[InstanceId,State.Name]' \
    --output text)

if [ -n "$REMAINING" ]; then
    echo "WARNING: Some instances still exist:"
    echo "$REMAINING"
else
    echo "No instances found ✓"
fi

# Check S3
if aws s3 ls s3://$BUCKET 2>/dev/null; then
    echo "WARNING: S3 bucket still exists: $BUCKET"
else
    echo "S3 bucket deleted ✓"
fi

echo ""
echo "========================================"
echo "Cleanup Complete!"
echo "========================================"
echo ""
echo "Your local results are preserved in:"
echo "  /mnt/cvp-archive/10e24/"
echo ""
echo "IAM roles are preserved for future runs"
echo "To completely remove AWS setup, manually delete:"
echo "  - IAM Role: CVPipeWorkerRole"
echo "  - Instance Profile: CVPipeWorkerProfile"
echo ""
echo "Check final costs in AWS Billing Dashboard"
