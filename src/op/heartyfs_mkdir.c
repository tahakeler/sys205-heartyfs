#include "../heartyfs.h"

// ----------------------
// Bitmap Helper Functions
// ----------------------

// Check if a block is free in the bitmap
int is_block_free(char *bitmap, int block_index) {
    return !(bitmap[block_index / 8] & (1 << (block_index % 8)));
}

// Mark a block as used in the bitmap
void set_block_used(char *bitmap, int block_index) {
    bitmap[block_index / 8] |= (1 << (block_index % 8));
}

// Find the first free block in the bitmap starting from DATA_START_BLOCK
int find_free_block(char *bitmap) {
    for (int i = DATA_START_BLOCK; i < NUM_BLOCK; i++) {
        if (is_block_free(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

// ----------------------
// Main Function
// ----------------------

int main(int argc, char *argv[]) {
    // Validate argument count
    if (argc != 2) {
        fprintf(stderr, "Usage: heartyfs_mkdir <absolute_path>\n");
        return 1;
    }

    char *path = argv[1];

    // Ensure the path is absolute
    if (path[0] != '/') {
        fprintf(stderr, "Path must be absolute.\n");
        return 1;
    }

    // Open the disk file and map it to memory
    int fd = open(DISK_FILE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open disk file");
        return 1;
    }

    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Memory mapping failed");
        close(fd);
        return 1;
    }

    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;
    char *bitmap = (char *)disk + BLOCK_SIZE;

    struct heartyfs_directory *current = root;
    int current_block = 0;

    // Tokenize the input path using "/"
    char *token = strtok(path, "/");
    while (token != NULL) {
        int found = 0;

        // Search for the token in the current directory entries
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                current_block = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)disk + current_block * BLOCK_SIZE);
                found = 1;
                break;
            }
        }

        // Peek at the next token to determine if this is the last level
        char *next = strtok(NULL, "/");

        // If this is the last level and the directory doesn't exist, create it
        if (!next) {
            if (found) {
                fprintf(stderr, "Directory already exists.\n");
                return 1;
            }

            if (current->size >= MAX_ENTRIES) {
                fprintf(stderr, "Directory full.\n");
                return 1;
            }

            int new_block = find_free_block(bitmap);
            if (new_block == -1) {
                fprintf(stderr, "No free blocks available.\n");
                return 1;
            }

            set_block_used(bitmap, new_block);

            // Initialize new directory block
            struct heartyfs_directory *new_dir = (struct heartyfs_directory *)((char *)disk + new_block * BLOCK_SIZE);
            new_dir->type = TYPE_DIR;
            strcpy(new_dir->name, token);
            new_dir->size = 2;

            // Add "." and ".." entries
            strcpy(new_dir->entries[0].file_name, ".");
            new_dir->entries[0].block_id = new_block;
            strcpy(new_dir->entries[1].file_name, "..");
            new_dir->entries[1].block_id = current_block;

            // Link the new directory in the parent
            strcpy(current->entries[current->size].file_name, token);
            current->entries[current->size].block_id = new_block;
            current->size++;

            // Sync changes to disk
            msync(disk, DISK_SIZE, MS_SYNC);
            printf("Directory created: %s\n", token);
            return 0;
        }

        // If this is not the last level but the token wasn't found, report an error
        if (!found) {
            fprintf(stderr, "Intermediate path not found: %s\n", token);
            return 1;
        }

        token = next;
    }

    return 0;
}
