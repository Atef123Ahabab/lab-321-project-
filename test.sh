#!/bin/bash

# VSFS Journaling Test Script
# This script demonstrates the metadata journaling functionality

set -e  # Exit on error

DISK_IMAGE="test.img"
VSFS="./vsfs"
MKFS="./mkfs.vsfs"

echo "========================================="
echo "VSFS Metadata Journaling Test Suite"
echo "========================================="
echo ""

# Clean up previous test
if [ -f "$DISK_IMAGE" ]; then
    echo "Cleaning up previous test..."
    rm -f "$DISK_IMAGE"
fi

# Create and format disk
echo "Step 1: Creating and formatting disk image..."
$MKFS "$DISK_IMAGE"
echo ""

# Show initial state
echo "Step 2: Initial file system state"
echo "-----------------------------------"
$VSFS "$DISK_IMAGE" stat
$VSFS "$DISK_IMAGE" ls
$VSFS "$DISK_IMAGE" check
echo ""

# Create files (write to journal only)
echo "Step 3: Creating first file (journaled, not installed)"
echo "--------------------------------------------------------"
$VSFS "$DISK_IMAGE" create file1.txt
echo ""

# Show state before install (file should NOT appear yet)
echo "Step 4: File system before install (file NOT visible yet)"
echo "-----------------------------------------------------------"
$VSFS "$DISK_IMAGE" ls
echo ""

# Install journal
echo "Step 5: Installing journal transactions"
echo "----------------------------------------"
$VSFS "$DISK_IMAGE" install
echo ""

# Show state after install
echo "Step 6: File system after install (file NOW visible)"
echo "------------------------------------------------------"
$VSFS "$DISK_IMAGE" ls
$VSFS "$DISK_IMAGE" stat
$VSFS "$DISK_IMAGE" check
echo ""

# Create more files - each needs its own install due to journal size
echo "Step 7: Creating more files (one at a time)"
echo "--------------------------------------------"
$VSFS "$DISK_IMAGE" create file2.txt
$VSFS "$DISK_IMAGE" install
$VSFS "$DISK_IMAGE" create file3.txt
$VSFS "$DISK_IMAGE" install
echo ""

# Final state
echo "Step 8: Final file system state"
echo "--------------------------------"
$VSFS "$DISK_IMAGE" ls
$VSFS "$DISK_IMAGE" stat
$VSFS "$DISK_IMAGE" check
echo ""

# Test multiple creates before install
echo "Step 9: Testing multiple transactions"
echo "--------------------------------------"
echo "(Note: With 16-block journal and 11 blocks/transaction,"
echo " we can only buffer 1 transaction before install)"
$VSFS "$DISK_IMAGE" create alpha.txt
echo "Journal now full, must install before next create..."
$VSFS "$DISK_IMAGE" install
$VSFS "$DISK_IMAGE" create beta.txt
$VSFS "$DISK_IMAGE" install
$VSFS "$DISK_IMAGE" ls
$VSFS "$DISK_IMAGE" check
echo ""

echo "========================================="
echo "All tests completed successfully!"
echo "========================================="
