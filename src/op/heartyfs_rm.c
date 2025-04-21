#include "../heartyfs.h"

// ----------------------
// Bitmap Helper Functions
// ----------------------

void set_block_free(char *bitmap, int block_index) {
    bitmap[block_index / 8] &= ~(1 << (block_index % 8));
}

// ----------------------
// Main Function
// ----------------------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: heartyfs_rm <file_path>\n");
        return 1;
    }

    char *path = argv[1];

    // Open and map the disk file
    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        return 1;
    }

    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Failed to mmap disk");
        close(fd);
        return 1;
    }

    char *bitmap = (char *)disk + BLOCK_SIZE;
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;

    char temp[256];
    strcpy(temp, path);

    // Traverse path to reach parent directory
    char *dir = strtok(temp, "/");
    struct heartyfs_directory *current = root;
    int inode_block = -1;
    char *next, *filename = NULL;

    while ((next = strtok(NULL, "/")) != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, dir) == 0) {
                current = (struct heartyfs_directory *)((char *)disk + current->entries[i].block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Path not found: %s\n", dir);
            return 1;
        }
        dir = next;
    }

    filename = dir;

    // Search for file entry and remove it
    for (int i = 0; i < current->size; i++) {
        if (strcmp(current->entries[i].file_name, filename) == 0) {
            inode_block = current->entries[i].block_id;

            // Free associated data blocks
            struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)disk + inode_block * BLOCK_SIZE);
            for (int j = 0; j < inode->size; j++) {
                set_block_free(bitmap, inode->data_blocks[j]);
            }

            // Free inode block itself
            set_block_free(bitmap, inode_block);

            // Remove file entry from directory
            for (int j = i; j < current->size - 1; j++) {
                current->entries[j] = current->entries[j + 1];
            }
            current->size--;

            msync(disk, DISK_SIZE, MS_SYNC);
            printf("File removed: %s\n", filename);
            return 0;
        }
    }

    fprintf(stderr, "File not found: %s\n", filename);
    return 1;
}
