#!/bin/bash
# Simple demonstration of VSFS journaling

set -e

echo "================================================="
echo "VSFS Metadata Journaling - Quick Demo"
echo "================================================="
echo ""

# Create fresh disk
echo "1. Creating fresh disk image..."
./mkfs.vsfs demo.img
echo ""

# Show initial state
echo "2. Initial state (empty filesystem):"
./vsfs demo.img ls
echo ""

# Create a file (journaled)
echo "3. Creating file 'hello.txt' (logged to journal)..."
./vsfs demo.img create hello.txt
echo ""

# Files not visible yet!
echo "4. Before install - file NOT visible yet:"
./vsfs demo.img ls
echo ""

# Install
echo "5. Installing journal (applying changes)..."
./vsfs demo.img install
echo ""

# Now it's visible
echo "6. After install - file IS visible:"
./vsfs demo.img ls
echo ""

# Verify consistency
echo "7. Verifying filesystem consistency:"
./vsfs demo.img check
echo ""

echo "================================================="
echo "Demo complete! The journaling ensures that file"
echo "creation is atomic - either fully applied or not"
echo "at all, providing crash consistency."
echo "================================================="
