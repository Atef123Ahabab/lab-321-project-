# VSFS Metadata Journaling - Project Summary

## ✅ What Was Built

A complete **metadata journaling system** for VSFS (Very Simple File System) that provides **crash consistency** through write-ahead logging.

## 🎯 Core Functionality

### 1. **create(filename)** - Write-Ahead Logging
- Computes metadata changes needed for file creation
- Logs changes to journal WITHOUT modifying actual filesystem
- Writes DATA records for each modified block:
  - Inode bitmap
  - Data bitmap  
  - Inode table (2 blocks)
  - Root directory
- Writes COMMIT record to mark transaction complete
- **Key insight**: Files are NOT visible until `install()` is called

### 2. **install()** - Transaction Replay
- Scans journal for completed transactions
- Applies each DATA record to its destination block
- Only processes transactions with COMMIT markers
- Clears journal after successful application
- **Key insight**: Incomplete transactions (no COMMIT) are safely discarded

## 📦 Project Structure

```
/tmp/vsfs-journaling/
├── vsfs.h           # Data structures (superblock, inode, journal records)
├── disk.c/h         # Low-level disk I/O and bitmap operations
├── journal.c/h      # Core journaling implementation
├── main.c           # CLI tool (create, install, ls, stat, check)
├── mkfs.c           # Disk formatter
├── Makefile         # Build system
├── test.sh          # Comprehensive test suite
├── demo.sh          # Quick demonstration
└── README.md        # Full documentation
```

## 🔧 How to Use

### Build
```bash
cd /tmp/vsfs-journaling
make
```

### Create Disk Image
```bash
./mkfs.vsfs mydisk.img
```

### Create Files (Journaled)
```bash
./vsfs mydisk.img create file1.txt
./vsfs mydisk.img create file2.txt  # Will fail - journal full!
```

### Install Transactions
```bash
./vsfs mydisk.img install
```

### Verify
```bash
./vsfs mydisk.img ls      # List files
./vsfs mydisk.img stat    # Show statistics
./vsfs mydisk.img check   # Validate consistency
```

### Run Tests
```bash
./test.sh    # Comprehensive test suite
./demo.sh    # Quick demonstration
```

## 🏗️ Disk Layout

```
Block 0:       Superblock (magic number, metadata)
Blocks 1-16:   Journal (16 blocks for write-ahead log)
Block 17:      Inode bitmap (tracks allocated inodes)
Block 18:      Data bitmap (tracks allocated data blocks)
Blocks 19-20:  Inode table (64 inodes max, 32 per block)
Blocks 21-84:  Data blocks (64 blocks for file data)
Total: 85 blocks × 4096 bytes = 348,160 bytes
```

## 📝 Journal Format

Each transaction consists of:
```
[HEADER:DATA][DATA:4096 bytes]  ← Inode bitmap
[HEADER:DATA][DATA:4096 bytes]  ← Data bitmap
[HEADER:DATA][DATA:4096 bytes]  ← Inode table block 0
[HEADER:DATA][DATA:4096 bytes]  ← Inode table block 1
[HEADER:DATA][DATA:4096 bytes]  ← Root directory
[HEADER:COMMIT]                  ← Transaction marker
```

**Space usage**: 5 DATA records × 2 blocks + 1 COMMIT = **11 blocks per transaction**

With 16 journal blocks, we can buffer **1 complete transaction** before needing to install.

## 🛡️ Crash Consistency

| Crash Point | Result | Why Safe? |
|------------|---------|-----------|
| Before DATA records | No change | Journal empty |
| During DATA writes | No change | No COMMIT yet |
| After some DATA | No change | Incomplete transaction |
| Before COMMIT | No change | Transaction ignored |
| After COMMIT | Applied on install | Fully recoverable |
| During install | Idempotent | Can re-run install |

## ✨ Key Features

✅ **Atomicity**: Files are created all-or-nothing  
✅ **Durability**: Committed changes survive crashes  
✅ **Consistency**: Validator checks (no dangling pointers, no leaks)  
✅ **Idempotence**: Safe to re-apply journal  
✅ **Crash Recovery**: Incomplete transactions automatically discarded  

## 🧪 Testing Results

All tests pass successfully:
- ✅ File creation with journaling
- ✅ Install applies transactions correctly
- ✅ Files invisible before install, visible after
- ✅ Consistency validation passes
- ✅ Journal clears after install
- ✅ Multiple create/install cycles work

## 📊 Performance Characteristics

- **Write Amplification**: Each file creation writes ~11 journal blocks + 5 actual blocks = 16 blocks total
- **Journal Capacity**: 1 transaction (with current layout)
- **Recovery Time**: O(journal_size) - scans all journal blocks once

## 🚀 Possible Extensions

1. **Larger Journal**: Increase JOURNAL_BLOCKS to buffer more transactions
2. **Checkpointing**: Track last committed transaction to avoid full scan
3. **Block Deduplication**: Journal only changed blocks, not full inode table
4. **Ordered Journaling**: Journal data blocks too, not just metadata
5. **Asynchronous Install**: Background thread to install periodically
6. **File Deletion**: Implement delete() with journaling
7. **Write Operations**: Support writing data to files

## 🎓 Educational Value

This project demonstrates:
- Write-ahead logging (WAL) used in databases and filesystems
- Transaction management (DATA + COMMIT protocol)
- Crash recovery mechanisms
- Metadata consistency validation
- File system internals (inodes, bitmaps, directories)

## 📝 Notes

- This is a **metadata journaling** system - only structure changes are logged
- **Limitation**: Journal size limits buffered transactions
- **Design Choice**: Simple 2-block DATA record format (header + full block)
- **Trade-off**: Safety vs. performance (every create needs install for now)

## 🏆 Success Criteria Met

✅ Implemented `create()` with write-ahead logging  
✅ Implemented `install()` with transaction replay  
✅ Journal format correctly handles DATA + COMMIT  
✅ Consistency validator passes all checks  
✅ Crash recovery through partial transaction handling  
✅ Complete test suite demonstrates correctness  

---

**This implementation provides the foundation for understanding how production file systems like ext3/ext4 (Linux) and NTFS (Windows) maintain consistency in the face of system crashes.**
