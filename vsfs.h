#ifndef VSFS_H
#define VSFS_H

#include <stdint.h>
#include <string.h>

// Block size
#define BLOCK_SIZE 4096

// Disk layout constants
#define SUPERBLOCK_BLOCK 0
#define JOURNAL_START 1
#define JOURNAL_BLOCKS 16
#define INODE_BITMAP_BLOCK 17
#define DATA_BITMAP_BLOCK 18
#define INODE_TABLE_START 19
#define INODE_TABLE_BLOCKS 2
#define DATA_BLOCKS_START 21
#define DATA_BLOCKS_COUNT 64

// File system limits
#define MAX_INODES 64
#define MAX_FILES MAX_INODES
#define MAX_FILENAME 28
#define DIRECT_POINTERS 12

// Journal record types
#define JOURNAL_DATA 1
#define JOURNAL_COMMIT 2

// File types
#define T_DIR 1
#define T_FILE 2

// Superblock structure
typedef struct {
    uint32_t magic;           // Magic number to identify VSFS
    uint32_t num_blocks;      // Total number of blocks
    uint32_t num_inodes;      // Total number of inodes
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t data_blocks_start;
} superblock_t;

// Inode structure
typedef struct {
    uint32_t size;            // File size in bytes
    uint16_t type;            // T_DIR or T_FILE
    uint16_t nlink;           // Number of links
    uint32_t blocks[DIRECT_POINTERS];  // Direct block pointers
} inode_t;

// Directory entry
typedef struct {
    char name[MAX_FILENAME];  // File name
    uint32_t inum;            // Inode number (0 = unused entry)
} dirent_t;

// Journal record header
typedef struct {
    uint32_t type;            // JOURNAL_DATA or JOURNAL_COMMIT
    uint32_t block_num;       // For DATA: destination block number
    uint32_t size;            // Size of data following header
} journal_header_t;

// Helper macros
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode_t))
#define DIRENTS_PER_BLOCK (BLOCK_SIZE / sizeof(dirent_t))

#endif // VSFS_H
