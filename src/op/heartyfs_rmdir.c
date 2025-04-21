#include "../heartyfs.h"

// ----------------------
// Main Function
// ----------------------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: heartyfs_rmdir <directory_path>\n");
        return 1;
    }

    char *path = argv[1];

    // Open and map the virtual disk
    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk");
        return 1;
    }

    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Failed to map disk");
        close(fd);
        return 1;
    }

    char *bitmap = (char *)disk + BLOCK_SIZE;
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;

    char temp_path[256];
    strcpy(temp_path, path);

    // Traverse the path
    char *token = strtok(temp_path, "/");
    struct heartyfs_directory *parent = root, *target = NULL;
    int parent_block = 0, target_block = 0;
    char target_name[MAX_FILENAME];

    while (token != NULL) {
        strcpy(target_name, token);
        int found = 0;

        // Search for matching directory
        for (int i = 0; i < parent->size; i++) {
            if (strcmp(parent->entries[i].file_name, token) == 0) {
                target_block = parent->entries[i].block_id;
                target = (struct heartyfs_directory *)((char *)disk + target_block * BLOCK_SIZE);
                found = 1;
                break;
            }
        }

        char *next = strtok(NULL, "/");
        if (!found) {
            fprintf(stderr, "Directory not found: %s\n", token);
            return 1;
        }

        if (!next) break;

        parent = target;
        parent_block = target_block;
        token = next;
    }

    // Prevent deletion if directory is not empty
    if (target->size > 2) {
        fprintf(stderr, "Directory not empty.\n");
        return 1;
    }

    // Remove entry from parent directory
    int found = 0;
    for (int i = 0; i < parent->size; i++) {
        if (strcmp(parent->entries[i].file_name, target_name) == 0) {
            found = 1;
            for (int j = i; j < parent->size - 1; j++) {
                parent->entries[j] = parent->entries[j + 1];
            }
            parent->size--;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "Entry not found.\n");
        return 1;
    }

    // Mark the directory block as free in the bitmap
    bitmap[target_block / 8] &= ~(1 << (target_block % 8));

    msync(disk, DISK_SIZE, MS_SYNC);
    printf("Directory removed: %s\n", target_name);
    return 0;
}
