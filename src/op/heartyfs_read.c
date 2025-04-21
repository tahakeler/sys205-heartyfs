#include "../heartyfs.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: heartyfs_read <file_path>\n");
        return 1;
    }

    char *path = argv[1];
    int fd = open(DISK_FILE_PATH, O_RDWR);
    void *disk = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;

    char temp[256];
    strcpy(temp, path);
    char *token = strtok(temp, "/");
    struct heartyfs_directory *current = root;
    int inode_block = -1;
    char *next, *filename;

    while ((next = strtok(NULL, "/")) != NULL) {
        int found = 0;
        for (int i = 0; i < current->size; i++) {
            if (strcmp(current->entries[i].file_name, token) == 0) {
                current = (struct heartyfs_directory *)((char *)disk + current->entries[i].block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Path not found: %s\n", token);
            return 1;
        }
        token = next;
    }
    filename = token;

    for (int i = 0; i < current->size; i++) {
        if (strcmp(current->entries[i].file_name, filename) == 0) {
            inode_block = current->entries[i].block_id;
            break;
        }
    }
    if (inode_block == -1) {
        fprintf(stderr, "File not found: %s\n", filename);
        return 1;
    }

    struct heartyfs_inode *inode = (struct heartyfs_inode *)((char *)disk + inode_block * BLOCK_SIZE);
    for (int i = 0; i < inode->size; i++) {
        struct heartyfs_data_block *db = (struct heartyfs_data_block *)((char *)disk + inode->data_blocks[i] * BLOCK_SIZE);
        fwrite(db->data, 1, db->size, stdout);
    }

    printf("\n");
    return 0;
}
