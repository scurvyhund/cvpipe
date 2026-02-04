#!/bin/bash
# 01-storage-setup.sh - Configure 20TB Seagate for CVPipe
# Run as: sudo ./01-storage-setup.sh /dev/sdX

set -e

DRIVE=$1
ARCHIVE_SIZE="90%"  # 18TB for archive
WORK_SIZE="100%"    # 2TB for working

echo "========================================"
echo "CVPipe Storage Setup"
echo "========================================"
echo ""

if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: Must run as root (use sudo)"
    exit 1
fi

if [ -z "$DRIVE" ]; then
    echo "Usage: sudo $0 /dev/sdX"
    echo ""
    echo "Available drives:"
    lsblk -d -o NAME,SIZE,MODEL | grep -v loop
    exit 1
fi

if [ ! -b "$DRIVE" ]; then
    echo "ERROR: $DRIVE is not a block device"
    exit 1
fi

echo "WARNING: This will ERASE all data on $DRIVE"
echo ""
lsblk -o NAME,SIZE,MODEL,FSTYPE $DRIVE
echo ""
read -p "Continue? Type 'yes' to proceed: " confirm

if [ "$confirm" != "yes" ]; then
    echo "Aborted"
    exit 1
fi

echo ""
echo "=== Creating GPT partition table ==="
parted -s $DRIVE mklabel gpt

echo "=== Creating partitions ==="
parted -s $DRIVE mkpart primary xfs 0% $ARCHIVE_SIZE
parted -s $DRIVE mkpart primary xfs $ARCHIVE_SIZE $WORK_SIZE

# Give kernel time to register partitions
sleep 2

PART1="${DRIVE}1"
PART2="${DRIVE}2"

echo "=== Formatting with XFS ==="
mkfs.xfs -f -L "CVP_Archive" $PART1
mkfs.xfs -f -L "CVP_Working" $PART2

echo "=== Creating mount points ==="
mkdir -p /mnt/cvp-archive
mkdir -p /mnt/cvp-working

echo "=== Getting UUIDs ==="
UUID1=$(blkid -s UUID -o value $PART1)
UUID2=$(blkid -s UUID -o value $PART2)

echo "=== Backing up /etc/fstab ==="
cp /etc/fstab /etc/fstab.backup.$(date +%Y%m%d-%H%M%S)

echo "=== Adding to /etc/fstab ==="
if ! grep -q "$UUID1" /etc/fstab; then
    echo "UUID=$UUID1  /mnt/cvp-archive  xfs  defaults,noatime  0  2" >> /etc/fstab
    echo "Added CVP_Archive to fstab"
else
    echo "CVP_Archive already in fstab"
fi

if ! grep -q "$UUID2" /etc/fstab; then
    echo "UUID=$UUID2  /mnt/cvp-working  xfs  defaults,noatime  0  2" >> /etc/fstab
    echo "Added CVP_Working to fstab"
else
    echo "CVP_Working already in fstab"
fi

echo "=== Mounting filesystems ==="
mount /mnt/cvp-archive
mount /mnt/cvp-working

echo "=== Setting ownership ==="
chown -R $SUDO_USER:$SUDO_USER /mnt/cvp-archive
chown -R $SUDO_USER:$SUDO_USER /mnt/cvp-working

echo "=== Creating directory structure ==="
mkdir -p /mnt/cvp-archive/{10e24,10e25,10e26,logs,results}
mkdir -p /mnt/cvp-working/{candidates,stage1,temp}

echo ""
echo "========================================"
echo "Setup Complete!"
echo "========================================"
echo ""
df -h /mnt/cvp-archive /mnt/cvp-working
echo ""
echo "Archive partition: /mnt/cvp-archive (18TB)"
echo "Working partition: /mnt/cvp-working (2TB)"
echo ""
echo "Next step: Run ./02-install-aws-cli.sh"
