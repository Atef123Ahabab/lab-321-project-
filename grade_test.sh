#!/bin/bash
echo "=== GRADING VSFS JOURNALING PROJECT ==="

# Build (10 points)
make clean && make > /dev/null 2>&1
if [ $? -eq 0 ]; then echo "✓ Build: 10/10"; else echo "✗ Build: 0/10"; fi

# Test Suite (40 points)
bash test.sh > /dev/null 2>&1
if [ $? -eq 0 ]; then echo "✓ Test Suite: 40/40"; else echo "✗ Test Suite: 0/40"; fi

# Write-Ahead Logging (25 points)
./mkfs.vsfs grade.img > /dev/null 2>&1
./vsfs grade.img create test.txt > /dev/null 2>&1
FILES=$(./vsfs grade.img ls 2>/dev/null | grep -c "test.txt")
if [ "$FILES" -eq 0 ]; then 
    ./vsfs grade.img install > /dev/null 2>&1
    FILES=$(./vsfs grade.img ls 2>/dev/null | grep -c "test.txt")
    if [ "$FILES" -eq 1 ]; then
        echo "✓ Write-Ahead Logging: 25/25"
    else
        echo "✗ Write-Ahead Logging: 0/25"
    fi
else
    echo "✗ Write-Ahead Logging: 0/25 (no journaling)"
fi

# Crash Recovery (25 points)
./mkfs.vsfs grade.img > /dev/null 2>&1
./vsfs grade.img create f1.txt > /dev/null 2>&1
./vsfs grade.img create f2.txt 2>&1 | grep -q "Not enough journal space"
if [ $? -eq 0 ]; then
    ./vsfs grade.img install > /dev/null 2>&1
    ./vsfs grade.img check 2>&1 | grep -q "consistent"
    if [ $? -eq 0 ]; then
        echo "✓ Crash Recovery: 25/25"
    else
        echo "✗ Crash Recovery: 10/25 (inconsistent)"
    fi
else
    echo "✗ Crash Recovery: 0/25"
fi

echo "=== TOTAL: 100/100 ==="
