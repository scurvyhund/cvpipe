# CVPipe AWS Cloud Processing Setup
## Complete Guide for 10^24 Search

---

## OVERVIEW

This package contains everything you need to run CVPipe Stage 2-4 processing on AWS cloud infrastructure, processing 10^24 4n+1 candidates in parallel across 20 spot instances.

**Timeline:**
- AWS Account Setup: 1 hour
- Stage 1 (nitroIII): 2-3 days
- Cloud Processing: 4-8 hours wall-clock

**Estimated Cost:** $300-400

---

## QUICK START

```bash
# 1. Setup HD (run first)
sudo ./01-storage-setup.sh /dev/sdX

# 2. Create AWS account (manual - see STEP 1 below)

# 3. Install AWS CLI and configure
./02-install-aws-cli.sh

# 4. Setup AWS infrastructure
./03-aws-bootstrap.sh

# 5. Run Stage 1 locally
./04-run-stage1.sh

# 6. Deploy to cloud
./05-deploy-to-cloud.sh

# 7. Monitor progress
./06-monitor-progress.sh

# 8. Download results
./07-download-results.sh

# 9. Cleanup when done
./08-cleanup.sh
```

---

## STEP 1: Create AWS Account (Manual)

1. Go to https://aws.amazon.com → "Create an AWS Account"
2. Provide:
   - Email address
   - Password
   - AWS account name (e.g., "j-cvpipe")
3. Enter payment information (credit card)
   - They'll charge $1 for verification
4. Choose support plan: "Basic Support" (free)
5. Verify email and phone number

**CRITICAL: Secure Your Root Account**
- Sign in as root user
- Go to IAM Dashboard → "Activate MFA on your root account"
- Use phone app (Google Authenticator or Authy)
- Store recovery codes somewhere safe
- **NEVER use root account after this**

**Create IAM User:**
1. IAM Dashboard → Users → "Create user"
   - Username: j-cvpipe
2. Set permissions: AdministratorAccess
3. Create access key:
   - Use case: "Command Line Interface (CLI)"
   - Download the .csv file
   - **SAVE THIS FILE** - you cannot retrieve the secret key later

---

## STEP 2: Set Budget Alerts (BEFORE SPENDING)

1. Account menu (top right) → "Billing and Cost Management"
2. Left menu → "Budgets" → "Create budget"
3. Settings:
   - Type: Cost budget
   - Name: CVPipe-10e24
   - Period: Monthly
   - Amount: $500
4. Alerts:
   - 50% ($250)
   - 80% ($400)
   - 100% ($500)
5. Email: your email address

Also enable CloudWatch billing alerts:
- Billing Dashboard → Preferences → Enable

---

## STEP 3: Run Scripts

All scripts are numbered in order. Run them sequentially:

### 01-storage-setup.sh
Sets up your 20TB Seagate drive with XFS partitions.
```bash
sudo ./01-storage-setup.sh /dev/sdX
```
**Output:** 18TB archive + 2TB working partitions

---

### 02-install-aws-cli.sh
Installs AWS CLI and configures with your IAM credentials.
```bash
./02-install-aws-cli.sh
```
**You'll need:** Access Key ID and Secret from Step 1

---

### 03-aws-bootstrap.sh
Creates S3 bucket, IAM roles, and infrastructure.
```bash
./03-aws-bootstrap.sh
```
**Creates:**
- S3 bucket for data transfer
- IAM role for EC2 instances
- Test instance for validation

---

### 04-run-stage1.sh
Runs CVPipe Stage 1 on nitroIII to generate candidates.
```bash
./04-run-stage1.sh
```
**Runtime:** 2-3 days
**Output:** candidates-10e24.dat in /mnt/cvp-working/

---

### 05-deploy-to-cloud.sh
Packages CVPipe, uploads to S3, launches 20 workers.
```bash
./05-deploy-to-cloud.sh
```
**Launches:** 20x c7a.8xlarge spot instances (640 cores)

---

### 06-monitor-progress.sh
Watches completion status and costs.
```bash
./06-monitor-progress.sh
```
Press Ctrl+C to exit monitoring loop.

---

### 07-download-results.sh
Downloads all results and logs when complete.
```bash
./07-download-results.sh
```
**Output:** /mnt/cvp-archive/10e24/

---

### 08-cleanup.sh
Deletes S3 bucket and verifies no resources left running.
```bash
./08-cleanup.sh
```
**IMPORTANT:** Run this to avoid ongoing charges!

---

## COST BREAKDOWN

| Item | Estimated Cost |
|------|----------------|
| c7a.8xlarge spot instances (640 core-hours) | $280-340 |
| S3 storage + transfer | $5-10 |
| Test instance | $1 |
| **Total** | **$300-400** |

---

## TROUBLESHOOTING

**"Access denied" errors:**
- Check IAM user has AdministratorAccess policy
- Verify `aws configure` was run with correct credentials

**Spot instances not launching:**
- Check your spot price limit in 05-deploy-to-cloud.sh
- Try different region (us-west-2 is default)

**Budget exceeded:**
- All scripts check budget before launching expensive operations
- Monitor with: `./06-monitor-progress.sh`

**Instance failures:**
- Check logs: `aws s3 ls s3://YOUR_BUCKET/logs/`
- View CloudWatch logs in AWS Console

---

## FILES IN THIS PACKAGE

```
cvpipe-aws-setup/
├── README.md                    # This file
├── 01-storage-setup.sh          # HD partition setup
├── 02-install-aws-cli.sh        # Install AWS CLI
├── 03-aws-bootstrap.sh          # Setup AWS infrastructure
├── 04-run-stage1.sh             # Run Stage 1 locally
├── 05-deploy-to-cloud.sh        # Deploy to cloud
├── 06-monitor-progress.sh       # Monitor progress
├── 07-download-results.sh       # Download results
├── 08-cleanup.sh                # Cleanup resources
├── worker-script.sh             # EC2 instance boot script
└── helper-functions.sh          # Shared utility functions
```

---

## SUPPORT

If you run into issues:
1. Check AWS CloudWatch logs
2. Review S3 bucket contents: `aws s3 ls s3://YOUR_BUCKET/ --recursive`
3. Check running costs: `./06-monitor-progress.sh`

Good luck with the 10^24 search!

---

## SAFETY NOTES

- Budget alerts are set but not hard limits
- Always run 08-cleanup.sh when done
- Test with small instance first (included in bootstrap)
- Monitor costs daily during processing

---

Generated for j's converse prime research project
Date: January 26, 2025
