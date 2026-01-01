#include "journal.h"
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to find the next free position in journal
static int find_journal_end(void) {
    uint8_t block[BLOCK_SIZE];
    int current_block = JOURNAL_START;
    int offset = 0;
    
    // Scan through journal blocks to find first empty spot
    for (int i = 0; i < JOURNAL_BLOCKS; i++) {
        if (disk_read(JOURNAL_START + i, block) != 0) {
            return -1;
        }
        
        // Check if block is empty (all zeros)
        int is_empty = 1;
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (block[j] != 0) {
                is_empty = 0;
                break;
            }
        }
        
        if (is_empty) {
            return i;
        }
    }
    
    return JOURNAL_BLOCKS; // Journal full
}

// Helper function to write a journal record at a specific journal block offset
// For DATA records: header in one block, full 4K data in next block (uses 2 blocks)
// For COMMIT records: just header (uses 1 block)
static int write_journal_record_data(int journal_block_offset, uint32_t dest_block, void *data) {
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, BLOCK_SIZE);
    
    // Write header block
    journal_header_t header;
    header.type = JOURNAL_DATA;
    header.block_num = dest_block;
    header.size = BLOCK_SIZE;
    
    memcpy(block, &header, sizeof(journal_header_t));
    if (disk_write(JOURNAL_START + journal_block_offset, block) != 0) {
        return -1;
    }
    
    // Write full data block
    if (disk_write(JOURNAL_START + journal_block_offset + 1, data) != 0) {
        return -1;
    }
    
    return 0;
}

static int write_journal_commit(int journal_block_offset) {
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, BLOCK_SIZE);
    
    journal_header_t header;
    header.type = JOURNAL_COMMIT;
    header.block_num = 0;
    header.size = 0;
    
    memcpy(block, &header, sizeof(journal_header_t));
    return disk_write(JOURNAL_START + journal_block_offset, block);
}

// Create a new file (write to journal only)
int create(const char *filename) {
    uint8_t inode_bitmap[BLOCK_SIZE];
    uint8_t data_bitmap[BLOCK_SIZE];
    uint8_t root_dir_block[BLOCK_SIZE];
    inode_t inode_table[INODES_PER_BLOCK * INODE_TABLE_BLOCKS];
    
    printf("Creating file: %s\n", filename);
    
    // Read current state
    if (disk_read(INODE_BITMAP_BLOCK, inode_bitmap) != 0) return -1;
    if (disk_read(DATA_BITMAP_BLOCK, data_bitmap) != 0) return -1;
    
    // Read inode table
    for (int i = 0; i < INODE_TABLE_BLOCKS; i++) {
        if (disk_read(INODE_TABLE_START + i, 
                     ((uint8_t*)inode_table) + i * BLOCK_SIZE) != 0) {
            return -1;
        }
    }
    
    // Root inode is always inode 0
    inode_t *root_inode = &inode_table[0];
    
    // Read root directory data
    if (root_inode->blocks[0] == 0) {
        fprintf(stderr, "Error: Root directory has no data block\n");
        return -1;
    }
    if (disk_read(root_inode->blocks[0], root_dir_block) != 0) return -1;
    
    // Check if file already exists
    dirent_t *entries = (dirent_t *)root_dir_block;
    for (int i = 0; i < DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inum != 0 && strcmp(entries[i].name, filename) == 0) {
            fprintf(stderr, "Error: File '%s' already exists\n", filename);
            return -1;
        }
    }
    
    // Find free inode
    int free_inum = bitmap_find_free(inode_bitmap, MAX_INODES);
    if (free_inum < 0) {
        fprintf(stderr, "Error: No free inodes\n");
        return -1;
    }
    
    // Find free data block for the new file
    int free_data_block = bitmap_find_free(data_bitmap, DATA_BLOCKS_COUNT);
    if (free_data_block < 0) {
        fprintf(stderr, "Error: No free data blocks\n");
        return -1;
    }
    
    // Find free directory entry
    int free_dirent = -1;
    for (int i = 0; i < DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inum == 0) {
            free_dirent = i;
            break;
        }
    }
    if (free_dirent < 0) {
        fprintf(stderr, "Error: Directory full\n");
        return -1;
    }
    
    // Now create the journal entries
    int journal_pos = find_journal_end();
    if (journal_pos < 0) {
        fprintf(stderr, "Error: Failed to find journal position\n");
        return -1;
    }
    // Each DATA record takes 2 blocks, COMMIT takes 1
    // We need: 5 DATA records (10 blocks) + 1 COMMIT (1 block) = 11 blocks total
    if (journal_pos + 11 > JOURNAL_BLOCKS) {
        fprintf(stderr, "Error: Not enough journal space (need 11 blocks, have %d available)\n", 
                JOURNAL_BLOCKS - journal_pos);
        return -1;
    }
    
    printf("  Allocating inode %d, data block %d\n", free_inum, free_data_block);
    
    // Prepare modified blocks
    // 1. Inode bitmap
    bitmap_set(inode_bitmap, free_inum);
    
    // 2. Data bitmap
    bitmap_set(data_bitmap, free_data_block);
    
    // 3. Inode table - create new inode
    inode_t *new_inode = &inode_table[free_inum];
    memset(new_inode, 0, sizeof(inode_t));
    new_inode->type = T_FILE;
    new_inode->size = 0;
    new_inode->nlink = 1;
    new_inode->blocks[0] = DATA_BLOCKS_START + free_data_block;
    
    // 4. Root directory - add entry
    strncpy(entries[free_dirent].name, filename, MAX_FILENAME - 1);
    entries[free_dirent].name[MAX_FILENAME - 1] = '\0';
    entries[free_dirent].inum = free_inum;
    root_inode->size += sizeof(dirent_t);
    
    // Write journal records
    int journal_start = journal_pos;
    
    // DATA record 1: Inode bitmap
    if (write_journal_record_data(journal_pos, INODE_BITMAP_BLOCK, inode_bitmap) != 0) {
        fprintf(stderr, "Error: Failed to write inode bitmap to journal\n");
        return -1;
    }
    journal_pos += 2;
    
    // DATA record 2: Data bitmap
    if (write_journal_record_data(journal_pos, DATA_BITMAP_BLOCK, data_bitmap) != 0) {
        fprintf(stderr, "Error: Failed to write data bitmap to journal\n");
        return -1;
    }
    journal_pos += 2;
    
    // DATA records 3-4: Inode table blocks
    for (int i = 0; i < INODE_TABLE_BLOCKS; i++) {
        if (write_journal_record_data(journal_pos, INODE_TABLE_START + i,
                                     ((uint8_t*)inode_table) + i * BLOCK_SIZE) != 0) {
            fprintf(stderr, "Error: Failed to write inode table block %d to journal\n", i);
            return -1;
        }
        journal_pos += 2;
    }
    
    // DATA record 5: Root directory
    if (write_journal_record_data(journal_pos, root_inode->blocks[0], root_dir_block) != 0) {
        fprintf(stderr, "Error: Failed to write root directory to journal\n");
        return -1;
    }
    journal_pos += 2;
    
    // COMMIT record
    if (write_journal_commit(journal_pos) != 0) {
        fprintf(stderr, "Error: Failed to write commit record\n");
        return -1;
    }
    journal_pos += 1;
    
    printf("  Transaction logged to journal (blocks %d-%d)\n", 
           journal_start, journal_pos - 1);
    
    return 0;
}

// Install journaled transactions to the file system
int install(void) {
    uint8_t header_block[BLOCK_SIZE];
    uint8_t data_block[BLOCK_SIZE];
    journal_header_t *header;
    
    printf("Installing journal transactions...\n");
    
    int transactions = 0;
    int records_applied = 0;
    int journal_idx = 0;
    
    // Scan through journal
    while (journal_idx < JOURNAL_BLOCKS) {
        // Read journal block (header)
        if (disk_read(JOURNAL_START + journal_idx, header_block) != 0) {
            fprintf(stderr, "Error: Failed to read journal block %d\n", journal_idx);
            return -1;
        }
        
        header = (journal_header_t *)header_block;
        
        // Check if we've reached empty space
        if (header->type == 0) {
            break;
        }
        
        // Process based on type
        if (header->type == JOURNAL_DATA) {
            // Ensure we have room for data block
            if (journal_idx + 1 >= JOURNAL_BLOCKS) {
                fprintf(stderr, "Error: Incomplete DATA record at journal block %d\n", journal_idx);
                break;
            }
            
            // Read the data block (next journal block)
            if (disk_read(JOURNAL_START + journal_idx + 1, data_block) != 0) {
                fprintf(stderr, "Error: Failed to read data block at journal %d\n", journal_idx + 1);
                return -1;
            }
            
            uint32_t dest_block_num = header->block_num;
            printf("  Applying DATA record: block %d\n", dest_block_num);
            
            // Write the data to its destination
            if (disk_write(dest_block_num, data_block) != 0) {
                fprintf(stderr, "Error: Failed to write block %d\n", dest_block_num);
                return -1;
            }
            
            records_applied++;
            journal_idx += 2; // DATA record uses 2 blocks
            
        } else if (header->type == JOURNAL_COMMIT) {
            printf("  Found COMMIT record (transaction %d complete)\n", transactions + 1);
            transactions++;
            journal_idx += 1; // COMMIT uses 1 block
            
        } else {
            fprintf(stderr, "Warning: Unknown journal record type %d at block %d\n", 
                   header->type, journal_idx);
            break;
        }
    }
    
    // Clear the journal
    printf("Clearing journal...\n");
    memset(header_block, 0, BLOCK_SIZE);
    for (int i = 0; i < JOURNAL_BLOCKS; i++) {
        if (disk_write(JOURNAL_START + i, header_block) != 0) {
            fprintf(stderr, "Error: Failed to clear journal block %d\n", i);
            return -1;
        }
    }
    
    printf("Install complete: %d transactions, %d records applied\n", 
           transactions, records_applied);
    
    return 0;
}
