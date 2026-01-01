#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsfs.h"
#include "disk.h"

#define VSFS_MAGIC 0x56534653  // "VSFS"
#define TOTAL_BLOCKS 85

void create_disk_image(const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Failed to create disk image");
        exit(1);
    }
    
    // Create empty disk
    uint8_t zero_block[BLOCK_SIZE];
    memset(zero_block, 0, BLOCK_SIZE);
    
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        fwrite(zero_block, 1, BLOCK_SIZE, fp);
    }
    
    fclose(fp);
    printf("Created disk image: %s (%d blocks, %d bytes)\n", 
           filename, TOTAL_BLOCKS, TOTAL_BLOCKS * BLOCK_SIZE);
}

void format_vsfs(const char *filename) {
    if (disk_open(filename) != 0) {
        fprintf(stderr, "Error: Cannot open disk image\n");
        exit(1);
    }
    
    uint8_t block[BLOCK_SIZE];
    
    // 1. Write superblock
    memset(block, 0, BLOCK_SIZE);
    superblock_t *sb = (superblock_t *)block;
    sb->magic = VSFS_MAGIC;
    sb->num_blocks = TOTAL_BLOCKS;
    sb->num_inodes = MAX_INODES;
    sb->inode_bitmap_block = INODE_BITMAP_BLOCK;
    sb->data_bitmap_block = DATA_BITMAP_BLOCK;
    sb->inode_table_start = INODE_TABLE_START;
    sb->data_blocks_start = DATA_BLOCKS_START;
    
    if (disk_write(SUPERBLOCK_BLOCK, block) != 0) {
        fprintf(stderr, "Error: Failed to write superblock\n");
        exit(1);
    }
    printf("Wrote superblock\n");
    
    // 2. Clear journal
    memset(block, 0, BLOCK_SIZE);
    for (int i = 0; i < JOURNAL_BLOCKS; i++) {
        if (disk_write(JOURNAL_START + i, block) != 0) {
            fprintf(stderr, "Error: Failed to clear journal\n");
            exit(1);
        }
    }
    printf("Cleared journal (%d blocks)\n", JOURNAL_BLOCKS);
    
    // 3. Initialize inode bitmap (mark inode 0 for root)
    memset(block, 0, BLOCK_SIZE);
    bitmap_set(block, 0);  // Root inode
    if (disk_write(INODE_BITMAP_BLOCK, block) != 0) {
        fprintf(stderr, "Error: Failed to write inode bitmap\n");
        exit(1);
    }
    printf("Initialized inode bitmap\n");
    
    // 4. Initialize data bitmap (mark block 0 for root directory)
    memset(block, 0, BLOCK_SIZE);
    bitmap_set(block, 0);  // Root directory data block
    if (disk_write(DATA_BITMAP_BLOCK, block) != 0) {
        fprintf(stderr, "Error: Failed to write data bitmap\n");
        exit(1);
    }
    printf("Initialized data bitmap\n");
    
    // 5. Initialize inode table
    memset(block, 0, BLOCK_SIZE);
    inode_t *inodes = (inode_t *)block;
    
    // Create root inode (inode 0)
    inodes[0].type = T_DIR;
    inodes[0].size = 0;
    inodes[0].nlink = 1;
    inodes[0].blocks[0] = DATA_BLOCKS_START;  // First data block
    
    if (disk_write(INODE_TABLE_START, block) != 0) {
        fprintf(stderr, "Error: Failed to write inode table block 0\n");
        exit(1);
    }
    
    // Write second inode table block (empty)
    memset(block, 0, BLOCK_SIZE);
    if (disk_write(INODE_TABLE_START + 1, block) != 0) {
        fprintf(stderr, "Error: Failed to write inode table block 1\n");
        exit(1);
    }
    printf("Initialized inode table\n");
    
    // 6. Initialize root directory (empty)
    memset(block, 0, BLOCK_SIZE);
    if (disk_write(DATA_BLOCKS_START, block) != 0) {
        fprintf(stderr, "Error: Failed to write root directory\n");
        exit(1);
    }
    printf("Initialized root directory\n");
    
    // 7. Clear remaining data blocks
    memset(block, 0, BLOCK_SIZE);
    for (int i = 1; i < DATA_BLOCKS_COUNT; i++) {
        if (disk_write(DATA_BLOCKS_START + i, block) != 0) {
            fprintf(stderr, "Error: Failed to clear data block %d\n", i);
            exit(1);
        }
    }
    printf("Cleared data blocks\n");
    
    disk_close();
    
    printf("\nVSFS formatted successfully!\n");
    printf("  Superblock:    block %d\n", SUPERBLOCK_BLOCK);
    printf("  Journal:       blocks %d-%d (%d blocks)\n", 
           JOURNAL_START, JOURNAL_START + JOURNAL_BLOCKS - 1, JOURNAL_BLOCKS);
    printf("  Inode bitmap:  block %d\n", INODE_BITMAP_BLOCK);
    printf("  Data bitmap:   block %d\n", DATA_BITMAP_BLOCK);
    printf("  Inode table:   blocks %d-%d\n", INODE_TABLE_START, INODE_TABLE_START + INODE_TABLE_BLOCKS - 1);
    printf("  Data blocks:   blocks %d-%d (%d blocks)\n", 
           DATA_BLOCKS_START, DATA_BLOCKS_START + DATA_BLOCKS_COUNT - 1, DATA_BLOCKS_COUNT);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk_image>\n", argv[0]);
        fprintf(stderr, "Creates and formats a VSFS disk image\n");
        return 1;
    }
    
    const char *filename = argv[1];
    
    printf("Creating VSFS disk image: %s\n", filename);
    printf("========================================\n\n");
    
    create_disk_image(filename);
    printf("\n");
    format_vsfs(filename);
    
    return 0;
}
