#include "disk.h"
#include <stdlib.h>
#include <string.h>

FILE *disk_fp = NULL;

int disk_open(const char *filename) {
    disk_fp = fopen(filename, "r+b");
    if (!disk_fp) {
        perror("Failed to open disk image");
        return -1;
    }
    return 0;
}

void disk_close(void) {
    if (disk_fp) {
        fclose(disk_fp);
        disk_fp = NULL;
    }
}

int disk_read(uint32_t block_num, void *buffer) {
    if (!disk_fp) return -1;
    
    if (fseek(disk_fp, block_num * BLOCK_SIZE, SEEK_SET) != 0) {
        perror("disk_read: fseek failed");
        return -1;
    }
    
    size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, disk_fp);
    if (bytes_read != BLOCK_SIZE) {
        perror("disk_read: fread failed");
        return -1;
    }
    
    return 0;
}

int disk_write(uint32_t block_num, const void *buffer) {
    if (!disk_fp) return -1;
    
    if (fseek(disk_fp, block_num * BLOCK_SIZE, SEEK_SET) != 0) {
        perror("disk_write: fseek failed");
        return -1;
    }
    
    size_t bytes_written = fwrite(buffer, 1, BLOCK_SIZE, disk_fp);
    if (bytes_written != BLOCK_SIZE) {
        perror("disk_write: fwrite failed");
        return -1;
    }
    
    fflush(disk_fp);
    return 0;
}

int bitmap_get(uint8_t *bitmap, uint32_t index) {
    uint32_t byte_offset = index / 8;
    uint32_t bit_offset = index % 8;
    return (bitmap[byte_offset] >> bit_offset) & 1;
}

void bitmap_set(uint8_t *bitmap, uint32_t index) {
    uint32_t byte_offset = index / 8;
    uint32_t bit_offset = index % 8;
    bitmap[byte_offset] |= (1 << bit_offset);
}

void bitmap_clear(uint8_t *bitmap, uint32_t index) {
    uint32_t byte_offset = index / 8;
    uint32_t bit_offset = index % 8;
    bitmap[byte_offset] &= ~(1 << bit_offset);
}

int bitmap_find_free(uint8_t *bitmap, uint32_t max_bits) {
    for (uint32_t i = 0; i < max_bits; i++) {
        if (!bitmap_get(bitmap, i)) {
            return i;
        }
    }
    return -1;
}
