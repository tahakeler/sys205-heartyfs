#include "heartyfs.h"

int is_block_free(char *bitmap, int block_index) {
    return !(bitmap[block_index / 8] & (1 << (block_index % 8)));
}

void set_block_used(char *bitmap, int block_index) {
    bitmap[block_index / 8] |= (1 << (block_index % 8));
}

void set_block_free(char *bitmap, int block_index) {
    bitmap[block_index / 8] &= ~(1 << (block_index % 8));
}

int find_free_block(char *bitmap) {
    for (int i = DATA_START_BLOCK; i < NUM_BLOCK; i++) {
        if (is_block_free(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

int main() {
    // Open the disk file
    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Cannot open the disk file");
        exit(1);
    }

    // Map the disk file onto memory
    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Cannot mmap the disk file");
        exit(1);
    }

    // === Superblock (Block 0) Initialization ===
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;
    root->type = TYPE_DIR;
    strcpy(root->name, "/");
    root->size = 2;

    // Add "." and ".."
    root->entries[0].block_id = 0;
    strcpy(root->entries[0].file_name, ".");
    root->entries[1].block_id = 0;
    strcpy(root->entries[1].file_name, "..");

    // === Bitmap (Block 1) Initialization ===
    char *bitmap = (char *)disk + BLOCK_SIZE;
    memset(bitmap, 0x00, BLOCK_SIZE); // Mark all as used (0)

    // Mark all blocks after block 1 as free (set to 1)
    for (int i = DATA_START_BLOCK; i < NUM_BLOCK; i++) {
        set_block_free(bitmap, i);
    }

    // Sync and cleanup
    msync(disk, DISK_SIZE, MS_SYNC);
    munmap(disk, DISK_SIZE);
    close(fd);

    printf("heartyfs initialized successfully.\n");
    return 0;
}
