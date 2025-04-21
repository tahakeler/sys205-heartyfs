#ifndef HEARTYFS_H
#define HEARTYFS_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DISK_FILE_PATH "/tmp/heartyfs"
#define BLOCK_SIZE 512
#define DISK_SIZE (1 << 20)           // 1 MB
#define NUM_BLOCK (DISK_SIZE / BLOCK_SIZE) // 2048 blocks
#define BITMAP_BLOCK 1
#define DATA_START_BLOCK 2
#define MAX_ENTRIES 14
#define MAX_FILENAME 28

#define TYPE_FILE 0
#define TYPE_DIR  1

// Directory Entry: 32 bytes
struct heartyfs_dir_entry {
    int block_id;                   // 4 bytes
    char file_name[MAX_FILENAME];  // 28 bytes
};

// Superblock (also root directory): 512 bytes
struct heartyfs_directory {
    int type;                                     // 4 bytes
    char name[MAX_FILENAME];                     // 28 bytes
    int size;                                     // 4 bytes
    struct heartyfs_dir_entry entries[MAX_ENTRIES]; // 14 * 32 = 448 bytes
    char padding[BLOCK_SIZE - 4 - MAX_FILENAME - 4 - (MAX_ENTRIES * sizeof(struct heartyfs_dir_entry))]; // Padding to fit 512
};

// Inode Block: 512 bytes
struct heartyfs_inode {
    int type;               // 4 bytes
    char name[MAX_FILENAME];// 28 bytes
    int size;               // 4 bytes
    int data_blocks[119];   // 476 bytes (119 * 4 = 476)
};

// Data Block: 512 bytes
struct heartyfs_data_block {
    int size;               // 4 bytes
    char data[BLOCK_SIZE - 4]; // 508 bytes
};

// Bitmap helper
int is_block_free(char *bitmap, int block_index);
void set_block_used(char *bitmap, int block_index);
void set_block_free(char *bitmap, int block_index);
int find_free_block(char *bitmap);

#endif
