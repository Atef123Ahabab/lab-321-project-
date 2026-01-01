# VSFS Metadata Journaling Implementation

This project implements a metadata journaling system for VSFS (Very Simple File System), providing crash consistency through write-ahead logging.

## Overview

The journaling system ensures that file system operations are atomic and recoverable in case of system crashes. Changes are first logged to a journal, then applied to the actual file system.

## Architecture

### Disk Layout (85 blocks total, 4096 bytes/block)

```
Block 0:       Superblock
Blocks 1-16:   Journal (16 blocks)
Block 17:      Inode bitmap
Block 18:      Data bitmap
Blocks 19-20:  Inode table (64 inodes max)
Blocks 21-84:  Data blocks (64 blocks)
```

### Journal Format

The journal uses a simple record-based format:

- **DATA Record**: Contains a full block image + destination block number
- **COMMIT Record**: Marks transaction completion

Example journal layout:
```
[DATA][DATA][DATA][COMMIT][DATA][DATA][COMMIT]...
```

## Components

### Core Files

- **vsfs.h**: Data structures (superblock, inode, journal records)
- **disk.c/h**: Low-level disk I/O and bitmap operations
- **journal.c/h**: Main journaling implementation
  - `create(filename)`: Log file creation to journal
  - `install()`: Apply journal transactions to file system
- **main.c**: Command-line interface
- **mkfs.c**: Disk image creation and formatting utility

## Building

```bash
make
```

This builds two executables:
- `vsfs`: Main file system tool
- `mkfs.vsfs`: Disk formatter

## Usage

### Create a Disk Image

```bash
./mkfs.vsfs disk.img
```

### Create Files (Write-Ahead Logging)

```bash
./vsfs disk.img create file1.txt
./vsfs disk.img create file2.txt
./vsfs disk.img create file3.txt
```

These commands log the file creation operations to the journal but **do not modify** the actual file system yet.

### Install Journal Transactions

```bash
./vsfs disk.img install
```

This applies all completed transactions from the journal to the actual file system and clears the journal.

### Other Commands

```bash
# List files
./vsfs disk.img ls

# Show statistics
./vsfs disk.img stat

# Check consistency
./vsfs disk.img check
```

## How It Works

### Phase 1: CREATE (Write-Ahead Logging)

1. Determine what blocks need to change for file creation:
   - Inode bitmap (allocate inode)
   - Data bitmap (allocate data block)
   - Inode table (create new inode)
   - Root directory (add directory entry)

2. Write DATA records to journal for each modified block
3. Write COMMIT record to mark transaction complete
4. **Do NOT modify actual file system**

### Phase 2: INSTALL (Recovery/Apply)

1. Scan journal from start to end
2. Find complete transactions (DATA records followed by COMMIT)
3. Apply each DATA block to its destination
4. Ignore incomplete transactions (no COMMIT)
5. Clear journal

## Crash Consistency

The system maintains consistency through:

- **Atomicity**: Transactions are all-or-nothing (require COMMIT)
- **Idempotence**: Re-applying journal is safe
- **Crash Recovery**: Incomplete transactions are safely discarded

### Crash Scenarios

| Crash Point | Result |
|------------|---------|
| Before COMMIT | Transaction discarded (safe) |
| After COMMIT | Transaction applied on next install |
| During install | Idempotent - can re-run install |

## Validation

The `check` command validates:
- ✓ No dangling pointers (references to unallocated inodes/blocks)
- ✓ No leaks (allocated but unreachable inodes/blocks)
- ✓ No double allocations
- ✓ Bitmaps match actual usage

## Testing

### Quick Test

```bash
make test
```

### Comprehensive Test

```bash
./test.sh
```

This runs through multiple scenarios:
1. Create and install single transaction
2. Create multiple files before install
3. Verify consistency at each step

## Example Session

```bash
# Create disk
$ ./mkfs.vsfs test.img

# Create files (journaled)
$ ./vsfs test.img create alpha.txt
$ ./vsfs test.img create beta.txt
$ ./vsfs test.img create gamma.txt

# Files not visible yet
$ ./vsfs test.img ls
Total: 0 files

# Install journal
$ ./vsfs test.img install

# Files now visible
$ ./vsfs test.img ls
alpha.txt      1         0
beta.txt       2         0
gamma.txt      3         0
Total: 3 files

# Verify consistency
$ ./vsfs test.img check
✓ File system is consistent
```

## Implementation Details

### Journal Management

- Journal is append-only during CREATE
- Find next free journal position by scanning for empty blocks
- Journal size: 16 blocks (sufficient for multiple transactions)

### Block Modifications

For each file creation, we modify:
1. **Inode bitmap**: Set bit for new inode
2. **Data bitmap**: Set bit for new data block
3. **Inode table**: Initialize new inode structure
4. **Root directory**: Add directory entry

### Transaction Format

Each transaction consists of:
```
[HEADER:DATA][BLOCK_DATA:4096 bytes]  // Inode bitmap
[HEADER:DATA][BLOCK_DATA:4096 bytes]  // Data bitmap
[HEADER:DATA][BLOCK_DATA:4096 bytes]  // Inode table block 0
[HEADER:DATA][BLOCK_DATA:4096 bytes]  // Inode table block 1
[HEADER:DATA][BLOCK_DATA:4096 bytes]  // Root directory
[HEADER:COMMIT][SIZE:0]               // Transaction complete
```

## Limitations

- Only supports file creation (no deletion, writing)
- Root directory only (no subdirectories)
- Single-threaded
- Journal has fixed size (16 blocks)

## Future Enhancements

Possible extensions:
- File deletion support
- Write/read operations
- Subdirectory support
- Checkpointing (only journal new changes)
- Ordered journaling (data + metadata)
- Background journal commit

## Author

This implementation demonstrates the core concepts of journaling file systems as used in production systems like ext3/ext4 (Linux) and NTFS (Windows).

## License

Educational/Academic Use
