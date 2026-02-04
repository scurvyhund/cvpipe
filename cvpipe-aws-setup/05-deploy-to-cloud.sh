#!/bin/bash
# 05-deploy-to-cloud.sh - Deploy CVPipe to AWS and launch workers

set -e
source ./helper-functions.sh

echo "========================================"
echo "CVPipe Cloud Deployment"
echo "========================================"
echo ""

# Check prerequisites
check_file ~/.cvpipe-bucket "Run ./03-aws-bootstrap.sh first"
check_file ~/.cvpipe-ami "Run ./03-aws-bootstrap.sh first"
check_file /mnt/cvp-working/candidates-10e24.dat "Run ./04-run-stage1.sh first"

BUCKET=$(cat ~/.cvpipe-bucket)
AMI=$(cat ~/.cvpipe-ami)
REGION=$(aws configure get region)

echo "S3 Bucket: $BUCKET"
echo "AMI: $AMI"
echo "Region: $REGION"
echo ""

# Find CVPipe source
CVPIPE_DIR=""
for dir in ~/cvpipe ~/CVPipe ~/projects/cvpipe; do
    if [ -f "$dir/Makefile" ]; then
        CVPIPE_DIR="$dir"
        break
    fi
done

if [ -z "$CVPIPE_DIR" ]; then
    read -p "CVPipe source directory: " CVPIPE_DIR
fi

echo "CVPipe source: $CVPIPE_DIR"

echo ""
echo "=== Step 1: Package CVPipe source code ==="
cd $CVPIPE_DIR
tar czf /tmp/cvpipe.tar.gz \
    Makefile \
    *.c \
    *.h \
    README* 2>/dev/null || true

ls -lh /tmp/cvpipe.tar.gz

echo ""
echo "=== Step 2: Upload CVPipe to S3 ==="
aws s3 cp /tmp/cvpipe.tar.gz s3://$BUCKET/

echo ""
echo "=== Step 3: Upload candidates to S3 ==="
echo "This may take a few minutes..."
aws s3 cp /mnt/cvp-working/candidates-10e24.dat s3://$BUCKET/candidates.dat

echo ""
echo "=== Step 4: Verify uploads ==="
aws s3 ls s3://$BUCKET/ --human-readable

echo ""
echo "=== Step 5: Create worker script ==="
# Use the worker-script.sh from this package
sed "s/YOUR_BUCKET_NAME/$BUCKET/" worker-script.sh > /tmp/worker-script-final.sh

echo ""
echo "=== Step 6: Create launch template ==="
USER_DATA=$(base64 -w 0 /tmp/worker-script-final.sh)

# Delete old template if exists
aws ec2 delete-launch-template --launch-template-name CVPipeWorker-10e24 2>/dev/null || true

aws ec2 create-launch-template \
    --launch-template-name CVPipeWorker-10e24 \
    --version-description "10^24 processing" \
    --launch-template-data "{
        \"ImageId\": \"$AMI\",
        \"InstanceType\": \"c7a.8xlarge\",
        \"IamInstanceProfile\": {
            \"Name\": \"CVPipeWorkerProfile\"
        },
        \"UserData\": \"$USER_DATA\",
        \"InstanceMarketOptions\": {
            \"MarketType\": \"spot\",
            \"SpotOptions\": {
                \"MaxPrice\": \"0.65\",
                \"SpotInstanceType\": \"one-time\",
                \"InstanceInterruptionBehavior\": \"terminate\"
            }
        },
        \"TagSpecifications\": [{
            \"ResourceType\": \"instance\",
            \"Tags\": [
                {\"Key\": \"Name\", \"Value\": \"CVPipe-Worker\"},
                {\"Key\": \"Project\", \"Value\": \"10e24\"}
            ]
        }]
    }"

echo "Launch template created"

echo ""
echo "========================================"
echo "Ready to Launch Workers"
echo "========================================"
echo ""
echo "Configuration:"
echo "  - Instance type: c7a.8xlarge (32 vCPU)"
echo "  - Number of instances: 20"
echo "  - Total cores: 640"
echo "  - Max spot price: $0.65/hr"
echo "  - Estimated cost: $300-400"
echo "  - Estimated runtime: 4-8 hours"
echo ""
read -p "Launch 20 workers now? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
    echo "Deployment paused. Run this script again to launch."
    exit 0
fi

echo ""
echo "=== Launching 20 spot instances ==="
aws ec2 run-instances \
    --launch-template LaunchTemplateName=CVPipeWorker-10e24 \
    --count 20

echo ""
echo "========================================"
echo "Workers Launched!"
echo "========================================"
echo ""
echo "20 spot instances are being provisioned"
echo "They will:"
echo "  1. Start up (~2 minutes)"
echo "  2. Download and compile CVPipe (~5 minutes)"
echo "  3. Process candidates (~4-8 hours)"
echo "  4. Upload results to S3"
echo "  5. Terminate automatically"
echo ""
echo "Monitor progress: ./06-monitor-progress.sh"
echo ""
echo "Check costs anytime:"
echo "  ./06-monitor-progress.sh --cost"
