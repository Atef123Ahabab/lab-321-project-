#ifndef DISK_H
#define DISK_H

#include <stdio.h>
#include <stdint.h>
#include "vsfs.h"

// Global disk file pointer
extern FILE *disk_fp;

// Disk I/O functions
int disk_open(const char *filename);
void disk_close(void);
int disk_read(uint32_t block_num, void *buffer);
int disk_write(uint32_t block_num, const void *buffer);

// Bitmap operations
int bitmap_get(uint8_t *bitmap, uint32_t index);
void bitmap_set(uint8_t *bitmap, uint32_t index);
void bitmap_clear(uint8_t *bitmap, uint32_t index);
int bitmap_find_free(uint8_t *bitmap, uint32_t max_bits);

#endif // DISK_H
