#include "../heartyfs.h"

// ----------------------
// Bitmap Helper Functions
// ----------------------

int is_block_free(char *bitmap, int block_index) {
    return !(bitmap[block_index / 8] & (1 << (block_index % 8)));
}

void set_block_used(char *bitmap, int block_index) {
    bitmap[block_index / 8] |= (1 << (block_index % 8));
}

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
    if (argc != 2) {
        fprintf(stderr, "Usage: heartyfs_creat <file_path>\n");
        return 1;
    }

    char *path = argv[1];
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

    char temp[256];
    strcpy(temp, path);

    // Tokenize the file path
    char *dir = strtok(temp, "/");
    struct heartyfs_directory *current = root;
    int current_block = 0;
    char *next, *filename = NULL;

    while ((next = strtok(NULL, "/")) != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, dir) == 0) {
                current_block = current->entries[i].block_id;
                current = (struct heartyfs_directory *)((char *)disk + current_block * BLOCK_SIZE);
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

    if (current->size >= MAX_ENTRIES) {
        fprintf(stderr, "Directory is full.\n");
        return 1;
    }

    // Allocate an inode block for the file
    int inode_block = find_free_block(bitmap);
    if (inode_block == -1) {
        fprintf(stderr, "No free inode blocks.\n");
        return 1;
    }

    set_block_used(bitmap, inode_block);

    // Initialize the inode
    struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)disk + inode_block * BLOCK_SIZE);
    inode->type = TYPE_FILE;
    strcpy(inode->name, filename);
    inode->size = 0;

    // Add entry to parent directory
    current->entries[current->size].block_id = inode_block;
    strcpy(current->entries[current->size].file_name, filename);
    current->size++;

    msync(disk, DISK_SIZE, MS_SYNC);
    printf("File created: %s\n", filename);
    return 0;
}
