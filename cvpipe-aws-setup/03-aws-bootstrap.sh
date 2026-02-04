#!/bin/bash
# 03-aws-bootstrap.sh - Setup AWS infrastructure for CVPipe

set -e
source ./helper-functions.sh

echo "========================================"
echo "AWS Infrastructure Bootstrap"
echo "========================================"
echo ""

# Get AWS region
REGION=$(aws configure get region)
if [ -z "$REGION" ]; then
    echo "ERROR: AWS region not configured"
    echo "Run: aws configure"
    exit 1
fi
echo "Using region: $REGION"

# Create unique S3 bucket name
BUCKET="cvpipe-10e24-$(date +%s)"
echo "Bucket name: $BUCKET"

echo ""
echo "=== Creating S3 bucket ==="
aws s3 mb s3://$BUCKET --region $REGION

# Save bucket name for other scripts
echo $BUCKET > ~/.cvpipe-bucket
echo "Bucket created and saved to ~/.cvpipe-bucket"

echo ""
echo "=== Setting lifecycle policy (auto-delete after 30 days) ==="
cat > /tmp/lifecycle.json <<EOF
{
    "Rules": [
        {
            "Id": "DeleteAfter30Days",
            "Status": "Enabled",
            "Expiration": {
                "Days": 30
            },
            "Filter": {
                "Prefix": ""
            }
        }
    ]
}
EOF

aws s3api put-bucket-lifecycle-configuration \
    --bucket $BUCKET \
    --lifecycle-configuration file:///tmp/lifecycle.json

echo ""
echo "=== Creating IAM role for EC2 instances ==="

# Create trust policy
cat > /tmp/ec2-trust-policy.json <<EOF
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Principal": {"Service": "ec2.amazonaws.com"},
            "Action": "sts:AssumeRole"
        }
    ]
}
EOF

# Check if role already exists
if aws iam get-role --role-name CVPipeWorkerRole > /dev/null 2>&1; then
    echo "Role CVPipeWorkerRole already exists"
else
    aws iam create-role \
        --role-name CVPipeWorkerRole \
        --assume-role-policy-document file:///tmp/ec2-trust-policy.json
    echo "Role created"
fi

# Attach S3 policy
aws iam attach-role-policy \
    --role-name CVPipeWorkerRole \
    --policy-arn arn:aws:iam::aws:policy/AmazonS3FullAccess 2>/dev/null || true

# Create/update instance profile
if aws iam get-instance-profile --instance-profile-name CVPipeWorkerProfile > /dev/null 2>&1; then
    echo "Instance profile already exists"
else
    aws iam create-instance-profile \
        --instance-profile-name CVPipeWorkerProfile
    
    aws iam add-role-to-instance-profile \
        --instance-profile-name CVPipeWorkerProfile \
        --role-name CVPipeWorkerRole
    
    echo "Instance profile created"
    echo "Waiting 10 seconds for IAM propagation..."
    sleep 10
fi

echo ""
echo "=== Getting latest Amazon Linux 2023 AMI ==="
AMI=$(aws ec2 describe-images \
    --owners amazon \
    --filters "Name=name,Values=al2023-ami-2023.*-x86_64" \
    --query 'Images | sort_by(@, &CreationDate) | [-1].ImageId' \
    --output text)
echo "AMI ID: $AMI"
echo $AMI > ~/.cvpipe-ami

echo ""
echo "=== Launching test instance (t3.micro - $0.01/hr) ==="
cat > /tmp/test-userdata.sh <<'EOF'
#!/bin/bash
yum update -y
yum install -y gcc gmp-devel make git
echo "Test instance ready at $(date)" > /tmp/ready.txt
EOF

INSTANCE_ID=$(aws ec2 run-instances \
    --image-id $AMI \
    --instance-type t3.micro \
    --iam-instance-profile Name=CVPipeWorkerProfile \
    --user-data file:///tmp/test-userdata.sh \
    --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value=CVPipe-Test},{Key=Project,Value=cvpipe-test}]' \
    --query 'Instances[0].InstanceId' \
    --output text)

echo "Test instance launched: $INSTANCE_ID"
echo $INSTANCE_ID > ~/.cvpipe-test-instance

echo "Waiting for instance to start (this takes ~30 seconds)..."
aws ec2 wait instance-running --instance-ids $INSTANCE_ID

PUBLIC_IP=$(aws ec2 describe-instances \
    --instance-ids $INSTANCE_ID \
    --query 'Reservations[0].Instances[0].PublicIpAddress' \
    --output text)

echo ""
echo "========================================"
echo "Bootstrap Complete!"
echo "========================================"
echo ""
echo "S3 Bucket: $BUCKET"
echo "Test Instance: $INSTANCE_ID"
echo "Test Instance IP: $PUBLIC_IP"
echo ""
echo "The test instance is running ($0.01/hr)"
echo "You can terminate it with:"
echo "  aws ec2 terminate-instances --instance-ids $INSTANCE_ID"
echo ""
echo "Next step: Run ./04-run-stage1.sh"
