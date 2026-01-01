#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsfs.h"
#include "disk.h"
#include "journal.h"

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <disk_image> <command> [args...]\n", prog);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  create <filename>   - Create a new file (logs to journal)\n");
    fprintf(stderr, "  install             - Install journal transactions\n");
    fprintf(stderr, "  ls                  - List files in root directory\n");
    fprintf(stderr, "  stat                - Show file system statistics\n");
    fprintf(stderr, "  check               - Validate file system consistency\n");
}

void cmd_ls(void) {
    uint8_t inode_table_data[BLOCK_SIZE * INODE_TABLE_BLOCKS];
    uint8_t root_dir_block[BLOCK_SIZE];
    
    // Read inode table
    for (int i = 0; i < INODE_TABLE_BLOCKS; i++) {
        if (disk_read(INODE_TABLE_START + i, inode_table_data + i * BLOCK_SIZE) != 0) {
            fprintf(stderr, "Error: Failed to read inode table\n");
            return;
        }
    }
    
    inode_t *inode_table = (inode_t *)inode_table_data;
    inode_t *root = &inode_table[0];
    
    if (root->blocks[0] == 0) {
        fprintf(stderr, "Error: Root directory has no data block\n");
        return;
    }
    
    // Read root directory
    if (disk_read(root->blocks[0], root_dir_block) != 0) {
        fprintf(stderr, "Error: Failed to read root directory\n");
        return;
    }
    
    dirent_t *entries = (dirent_t *)root_dir_block;
    
    printf("Files in root directory:\n");
    printf("%-30s %10s %10s\n", "Name", "Inode", "Size");
    printf("-------------------------------------------------------\n");
    
    int count = 0;
    for (size_t i = 0; i < DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inum != 0) {
            inode_t *file_inode = &inode_table[entries[i].inum];
            printf("%-30s %10u %10u\n", entries[i].name, entries[i].inum, file_inode->size);
            count++;
        }
    }
    
    printf("\nTotal: %d files\n", count);
}

void cmd_stat(void) {
    uint8_t inode_bitmap[BLOCK_SIZE];
    uint8_t data_bitmap[BLOCK_SIZE];
    superblock_t sb;
    
    if (disk_read(SUPERBLOCK_BLOCK, &sb) != 0) {
        fprintf(stderr, "Error: Failed to read superblock\n");
        return;
    }
    
    if (disk_read(INODE_BITMAP_BLOCK, inode_bitmap) != 0) {
        fprintf(stderr, "Error: Failed to read inode bitmap\n");
        return;
    }
    
    if (disk_read(DATA_BITMAP_BLOCK, data_bitmap) != 0) {
        fprintf(stderr, "Error: Failed to read data bitmap\n");
        return;
    }
    
    // Count allocated inodes and blocks
    int used_inodes = 0;
    int used_blocks = 0;
    
    for (int i = 0; i < MAX_INODES; i++) {
        if (bitmap_get(inode_bitmap, i)) {
            used_inodes++;
        }
    }
    
    for (int i = 0; i < DATA_BLOCKS_COUNT; i++) {
        if (bitmap_get(data_bitmap, i)) {
            used_blocks++;
        }
    }
    
    printf("File System Statistics:\n");
    printf("  Magic:        0x%08x\n", sb.magic);
    printf("  Total blocks: %u\n", sb.num_blocks);
    printf("  Total inodes: %u\n", sb.num_inodes);
    printf("  Used inodes:  %d / %d\n", used_inodes, MAX_INODES);
    printf("  Used blocks:  %d / %d\n", used_blocks, DATA_BLOCKS_COUNT);
    printf("  Free inodes:  %d\n", MAX_INODES - used_inodes);
    printf("  Free blocks:  %d\n", DATA_BLOCKS_COUNT - used_blocks);
}

void cmd_check(void) {
    uint8_t inode_bitmap[BLOCK_SIZE];
    uint8_t data_bitmap[BLOCK_SIZE];
    uint8_t inode_table_data[BLOCK_SIZE * INODE_TABLE_BLOCKS];
    uint8_t root_dir_block[BLOCK_SIZE];
    
    printf("Checking file system consistency...\n");
    
    // Read bitmaps
    if (disk_read(INODE_BITMAP_BLOCK, inode_bitmap) != 0) return;
    if (disk_read(DATA_BITMAP_BLOCK, data_bitmap) != 0) return;
    
    // Read inode table
    for (int i = 0; i < INODE_TABLE_BLOCKS; i++) {
        if (disk_read(INODE_TABLE_START + i, inode_table_data + i * BLOCK_SIZE) != 0) {
            return;
        }
    }
    
    inode_t *inode_table = (inode_t *)inode_table_data;
    inode_t *root = &inode_table[0];
    
    int errors = 0;
    
    // Check root directory
    if (!bitmap_get(inode_bitmap, 0)) {
        printf("ERROR: Root inode not allocated in bitmap\n");
        errors++;
    }
    
    if (root->blocks[0] == 0) {
        printf("ERROR: Root directory has no data block\n");
        errors++;
        return;
    }
    
    // Read root directory
    if (disk_read(root->blocks[0], root_dir_block) != 0) return;
    dirent_t *entries = (dirent_t *)root_dir_block;
    
    // Check each file
    for (size_t i = 0; i < DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inum == 0) continue;
        
        uint32_t inum = entries[i].inum;
        
        // Check inode is in valid range
        if (inum >= MAX_INODES) {
            printf("ERROR: File '%s' has invalid inode %u\n", entries[i].name, inum);
            errors++;
            continue;
        }
        
        // Check inode is marked allocated
        if (!bitmap_get(inode_bitmap, inum)) {
            printf("ERROR: File '%s' inode %u not marked in bitmap (dangling pointer)\n", 
                   entries[i].name, inum);
            errors++;
        }
        
        // Check data blocks
        inode_t *inode = &inode_table[inum];
        for (int j = 0; j < DIRECT_POINTERS; j++) {
            if (inode->blocks[j] == 0) continue;
            
            // Check block is in valid range
            if (inode->blocks[j] < DATA_BLOCKS_START || 
                inode->blocks[j] >= DATA_BLOCKS_START + DATA_BLOCKS_COUNT) {
                printf("ERROR: File '%s' has invalid block pointer %u\n", 
                       entries[i].name, inode->blocks[j]);
                errors++;
                continue;
            }
            
            // Check block is marked allocated
            uint32_t data_block_idx = inode->blocks[j] - DATA_BLOCKS_START;
            if (!bitmap_get(data_bitmap, data_block_idx)) {
                printf("ERROR: File '%s' block %u not marked in bitmap\n", 
                       entries[i].name, inode->blocks[j]);
                errors++;
            }
        }
    }
    
    // Check for leaked inodes (allocated but not referenced)
    for (int i = 1; i < MAX_INODES; i++) { // Skip root (inode 0)
        if (!bitmap_get(inode_bitmap, i)) continue;
        
        int found = 0;
        for (size_t j = 0; j < DIRENTS_PER_BLOCK; j++) {
            if (entries[j].inum == (uint32_t)i) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("ERROR: Inode %d is allocated but not referenced (leak)\n", i);
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ File system is consistent\n");
    } else {
        printf("✗ Found %d error(s)\n", errors);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *disk_image = argv[1];
    const char *command = argv[2];
    
    // Open disk image
    if (disk_open(disk_image) != 0) {
        fprintf(stderr, "Error: Cannot open disk image '%s'\n", disk_image);
        return 1;
    }
    
    // Execute command
    int ret = 0;
    
    if (strcmp(command, "create") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: create requires a filename\n");
            print_usage(argv[0]);
            ret = 1;
        } else {
            ret = create(argv[3]);
        }
    } 
    else if (strcmp(command, "install") == 0) {
        ret = install();
    }
    else if (strcmp(command, "ls") == 0) {
        cmd_ls();
    }
    else if (strcmp(command, "stat") == 0) {
        cmd_stat();
    }
    else if (strcmp(command, "check") == 0) {
        cmd_check();
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        ret = 1;
    }
    
    disk_close();
    return ret;
}
