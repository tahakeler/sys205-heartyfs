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
    if (argc != 3) {
        fprintf(stderr, "Usage: heartyfs_write <fs_path> <host_path>\n");
        return 1;
    }

    char *fs_path = argv[1];
    char *host_path = argv[2];

    FILE *src = fopen(host_path, "r");
    if (!src) {
        perror("Failed to open source file");
        return 1;
    }

    fseek(src, 0, SEEK_END);
    long filesize = ftell(src);
    fseek(src, 0, SEEK_SET);

    if (filesize > 119 * 508) {
        fprintf(stderr, "File size exceeds heartyfs capacity.\n");
        fclose(src);
        return 1;
    }

    int fd = open(DISK_FILE_PATH, O_RDWR);
    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char *bitmap = (char *)disk + BLOCK_SIZE;
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;

    char temp[256];
    strcpy(temp, fs_path);
    char *dir = strtok(temp, "/");
    struct heartyfs_directory *current = root;
    int inode_block = -1;
    char *next, *filename = NULL;

    // Traverse to the directory
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
            fprintf(stderr, "Invalid path: %s\n", dir);
            return 1;
        }
        dir = next;
    }

    filename = dir;

    // Locate the inode for the target file
    for (int i = 0; i < current->size; i++) {
        if (strcmp(current->entries[i].file_name, filename) == 0) {
            inode_block = current->entries[i].block_id;
            break;
        }
    }

    if (inode_block == -1) {
        fprintf(stderr, "File not found in heartyfs: %s\n", filename);
        return 1;
    }

    struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)disk + inode_block * BLOCK_SIZE);
    int total_written = 0;
    char buffer[508];

    // Write the file into heartyfs in 508-byte chunks
    while (!feof(src)) {
        int block = find_free_block(bitmap);
        if (block == -1) break;

        set_block_used(bitmap, block);
        struct heartyfs_data_block *datablock = (struct heartyfs_data_block *)((char *)disk + block * BLOCK_SIZE);

        int bytes = fread(buffer, 1, 508, src);
        memcpy(datablock->data, buffer, bytes);
        datablock->size = bytes;

        inode->data_blocks[inode->size++] = block;
        total_written += bytes;
    }

    msync(disk, DISK_SIZE, MS_SYNC);
    fclose(src);
    printf("Wrote %d bytes to %s\n", total_written, filename);
    return 0;
}
