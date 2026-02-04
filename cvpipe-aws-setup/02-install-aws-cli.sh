#!/bin/bash
# 02-install-aws-cli.sh - Install and configure AWS CLI

set -e

echo "========================================"
echo "AWS CLI Installation & Configuration"
echo "========================================"
echo ""

# Check if already installed
if command -v aws &> /dev/null; then
    CURRENT_VERSION=$(aws --version 2>&1 | cut -d' ' -f1)
    echo "AWS CLI already installed: $CURRENT_VERSION"
    read -p "Reinstall? (y/n): " reinstall
    if [ "$reinstall" != "y" ]; then
        echo "Skipping installation"
        aws configure
        exit 0
    fi
fi

echo "=== Downloading AWS CLI v2 ==="
cd /tmp
curl -s "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"

echo "=== Extracting ==="
unzip -q awscliv2.zip

echo "=== Installing ==="
sudo ./aws/install --update

echo "=== Cleaning up ==="
rm -rf aws awscliv2.zip

echo "=== Verifying installation ==="
aws --version

echo ""
echo "========================================"
echo "AWS Configuration"
echo "========================================"
echo ""
echo "You'll need your IAM user credentials:"
echo "  - AWS Access Key ID"
echo "  - AWS Secret Access Key"
echo ""
echo "These were in the CSV file you downloaded when creating your IAM user."
echo ""
read -p "Press Enter when ready to configure..."

aws configure

echo ""
echo "=== Testing AWS connection ==="
if aws sts get-caller-identity > /dev/null 2>&1; then
    echo "SUCCESS! AWS CLI configured correctly"
    echo ""
    aws sts get-caller-identity
else
    echo "ERROR: Could not connect to AWS"
    echo "Check your credentials and try: aws configure"
    exit 1
fi

echo ""
echo "========================================"
echo "Configuration Complete!"
echo "========================================"
echo ""
echo "Next step: Run ./03-aws-bootstrap.sh"
