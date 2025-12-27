#include "shell_layer.h"
#include "../logic/logic_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Shell session state
static char* current_path;   // Current working directory (absolute VFS path)
static char* file_name;      // Host path to VFS container file

// Mount filesystem and initialize metadata
static int init(void)
{
    // Mount the VFS container file and initialize metadata caches
    const int res = fs_mount(file_name);

    if (!is_mounted()) {
        printf("Failed to mount file system.\n");
        printf("Try to format the fs with 'format <size MB>'\n");
        return res;
    }

    metadata_init();
    current_path = "/";
    return res;
}

// ================================================================
// 🔹 Shell main loop
// ================================================================

void run_shell(const char *filesystem_name)
{
    // Interactive REPL loop
    printf("=== Virtual File System Shell ===\n");
    printf("Type 'help' for list of supported commands.\n");

    file_name = strdup(filesystem_name);
    init();

    char input[1024];
    while (1)
    {
        // Prompt + read a single line command
        printf("vfs> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        // Strip trailing newline
        input[strcspn(input, "\n")] = 0;

        execute_command(input);
    }
}

// ================================================================
// 🔹 Helper functions
// ================================================================

/**
 * @brief Executes a single command line by parsing its arguments
 */
void execute_command(const char* input)
{
    // Parse up to 3 arguments (command + up to 3 parameters)
    char cmd[64], arg1[256], arg2[256], arg3[256];
    const int args = sscanf(input, "%63s %255s %255s %255s", cmd, arg1, arg2, arg3);
    if (args < 1) return;

    // When FS is not mounted, only 'format' is allowed
    if (!is_mounted()) {
        if (strcmp(cmd, "format") != 0) {
            printf("FS not mounted! Use 'format <sizeMB>' first.\n");
            return;
        }

        if (args < 2) {
            printf("Usage: format <sizeMB>\n");
            return;
        }

        const int size = (int)strtol(arg1, 0, 0);
        const int res = fs_format_cmd(size);
        printf(res == 0 ? "OK\n" : "CANNOT CREATE FILE\n");
        return;
    }

    // Command dispatch
    if (strcmp(cmd, "xcp") == 0) {
        if (args < 4) { printf("Usage: xcp s1 s2 s3\n"); return; }
        const int res = fs_xcp(arg1, arg2, arg3);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else if (res == 3) printf("FILE TOO LARGE\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "add") == 0) {
        if (args < 3) { printf("Usage: add s1 s2\n"); return; }
        const int res = fs_add(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 3) printf("FILE TOO LARGE\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "cp") == 0) {
        if (args < 3) { printf("Usage: cp s1 s2\n"); return; }
        const int res = fs_copy(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "mv") == 0) {
        if (args < 3) { printf("Usage: mv s1 s2\n"); return; }
        const int res = fs_move(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "rm") == 0) {
        if (args < 2) { printf("Usage: rm s1\n"); return; }
        const int res = fs_remove(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 2) printf("CANNOT REMOVE DIRECTORY\n");
        else printf("FILE NOT FOUND\n");
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (args < 2) { printf("Usage: mkdir a1\n"); return; }
        const int res = fs_mkdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("PATH NOT FOUND\n");
        else if (res == 2) printf("EXIST\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "rmdir") == 0) {
        if (args < 2) { printf("Usage: rmdir a1\n"); return; }
        const int res = fs_rmdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("NOT EMPTY\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "ls") == 0) {
        if (args == 1) (void)fs_ls(".");
        else if (fs_ls(arg1) != 0) printf("PATH NOT FOUND\n");
    }
    else if (strcmp(cmd, "cat") == 0) {
        if (args < 2) { printf("Usage: cat s1\n"); return; }
        const int res = fs_cat(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }
    else if (strcmp(cmd, "cd") == 0) {
        if (args < 2) { printf("Usage: cd a1\n"); return; }
        const int res = fs_cd(arg1);
        printf(res == 0 ? "OK\n" : "PATH NOT FOUND\n");
    }
    else if (strcmp(cmd, "pwd") == 0) {
        printf("Current path: %s\n", current_path);
    }
    else if (strcmp(cmd, "info") == 0) {
        if (args < 2) { printf("Usage: info s1\n"); return; }
        const int res = fs_info(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }
    else if (strcmp(cmd, "incp") == 0) {
        if (args < 3) { printf("Usage: incp s1 s2\n"); return; }
        const int res = fs_import(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "outcp") == 0) {
        if (args < 3) { printf("Usage: outcp s1 s2\n"); return; }
        const int res = fs_export(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else printf("UNKNOWN ERROR\n");
    }
    else if (strcmp(cmd, "load") == 0) {
        if (args < 2) { printf("Usage: load s1\n"); return; }
        const int res = fs_load_script(arg1);
        printf(res == 0 ? "OK\n" : "FILE NOT FOUND\n");
    }
    else if (strcmp(cmd, "format") == 0) {
        if (args < 2) { printf("Usage: format <sizeMB>\n"); return; }
        const int size = (int)strtol(arg1, 0, 0);
        const int res = fs_format_cmd(size);
        printf(res == 0 ? "OK\n" : "CANNOT CREATE FILE\n");
    }
    else if (strcmp(cmd, "exit") == 0) {
        // Ensure metadata is flushed before exit
        fs_unmount();
        printf("Exiting...\n");
        exit(0);
    }
    else if (strcmp(cmd, "statfs") == 0) {
        fs_stat();
    }
    else {
        printf("Unknown command\n");
    }
}

int fs_copy(char* src, char* dest) {
    src = complete_path(src);
    dest = complete_path(dest);

    char parent_path[MAX_PATH_LEN], src_name[FILENAME_MAX];
    if (!split_path(src, parent_path, src_name)) return 1;

    const int src_node = find_inode_by_path(src);

    if (src_node < 0) return 1;
    if (is_directory(src_node)) return 1;

    char dest_parent_path[MAX_PATH_LEN], dest_name[FILENAME_MAX];
    if (!split_path(dest, dest_parent_path, dest_name)) return 1;

    const int dest_parent_node = find_inode_by_path(dest_parent_path);

    if (dest_parent_node < 0) return 2;

    int new_inode_id = create_file(dest_parent_node, dest_name, false);
    if (new_inode_id < 0) return 3;

    void* buffer = malloc(MAX_FILE_SIZE);

    int bytes_read = read_inode_data(src_node, buffer);

    int bytes_written = write_inode_data(new_inode_id, buffer, bytes_read);

    if (bytes_written != bytes_read)
    {
        printf("WRITE ERROR (%d != %d)\n", bytes_written, bytes_read);
        return 3;
    }

    return 0;
}

int fs_move(char* src, char* dest) {
    // Move is implemented as: unlink from old parent + link into new parent.
    src = complete_path(src);
    dest = complete_path(dest);

    if (path_exists(dest)) return 2;

    char parent_path[MAX_PATH_LEN], src_name[FILENAME_MAX];
    if (!split_path(src, parent_path, src_name)) return 1;

    const int src_node = find_inode_by_path(src);

    char dest_parent_path[MAX_PATH_LEN], dest_name[FILENAME_MAX];
    if (!split_path(dest, dest_parent_path, dest_name)) return 1;

    const int dest_parent_node = find_inode_by_path(dest_parent_path);

    if (src_node < 0) return 1;
    if (dest_parent_node < 0) return 2;
    if (!is_directory(dest_parent_node)) return 2;

    // Use logical AND (not bitwise) to combine boolean results
    if (add_directory_item(dest_parent_node, dest_name, src_node) &&
        remove_directory_item(find_inode_by_path(parent_path), src_name)) {
        return 0;
    }

    return 3;
}

int fs_remove(char* path) {
    // rm removes only regular files; directories are handled by rmdir.
    path = complete_path(path);

    const int node_id = find_inode_by_path(path);
    if (node_id < 0) return 1;

    if (is_directory(node_id)) return 2;

    return delete_file(path);
}

int fs_mkdir(char* path) {
    // Create a directory and link it into its parent directory.
    path = complete_path(path);

    if (path_exists(path)) return 2; // directory already exists

    char parent_path[MAX_PATH_LEN];
    char dir_name[MAX_FILENAME_LEN];

    if (!split_path(path, parent_path, dir_name)) return 1;

    const int parent_node = find_inode_by_path(parent_path);
    if (parent_node < 0) return 1;

    const int new_inode = create_file(parent_node, dir_name, true);
    return (new_inode >= 0) ? 0 : 1;
}

int fs_rmdir(char* path) {

    path = complete_path(path);

    const int node_id = find_inode_by_path(path);

    if (node_id < 0) return 1;

    if (!is_directory(node_id)) return 2;

    if (!is_directory_empty(node_id)) return 2;

    return delete_file(path);
}

int fs_ls(char* path) {
    if (strcmp(path, ".") == 0) {
        path = current_path;
    } else {
        path = complete_path(path);
    }
    const int node_id = find_inode_by_path(path);
    if (node_id < 0) return 1;
    if (is_directory(node_id)) {
        printf("Containment of %s:\n", path);
        list_directory(node_id);
        return 0;
    }
    return 1;
}

int fs_cat(char* path) {
    // Resolve to absolute path inside VFS
    path = complete_path(path);

    // Locate inode
    const int inode_id = find_inode_by_path(path);
    if (inode_id < 0) return 1;
    if (is_directory(inode_id)) return 1;

    // Read file content
    char* buffer = (char*)malloc(MAX_FILE_SIZE + 1);
    if (!buffer) {
        printf("MEMORY ERROR\n");
        return 1;
    }

    const int read_bytes = read_inode_data(inode_id, buffer);
    if (read_bytes < 0) {
        free(buffer);
        return 1;
    }

    buffer[read_bytes] = '\0';
    printf("%s\n", buffer);

    free(buffer);
    return 0;
}

int fs_cd(char* path) {

    path = complete_path(path);

    int inode_id = find_inode_by_path(path);

    if (inode_id < 0) return 1;
    if (!is_directory(inode_id)) return 2;

    current_path = strdup(path);
    return 0;
}

void fs_pwd(void* buffer)
{
    buffer = current_path;
}

int fs_info(char* path)
{
    // Print basic inode info and referenced blocks.
    path = complete_path(path);

    const int inode_id = find_inode_by_path(path);
    if (inode_id < 0) {
        printf("FILE NOT FOUND\n");
        return 1;
    }

    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    // Extract basename for display
    const char* name = strrchr(path, '/');
    name = name ? (name + 1) : path;
    if (name[0] == '\0') name = "/";

    const int file_size = (int)inode.file_size;

    printf("%s - %d B - i-node %d - ", name, file_size, inode_id);
    printf(inode.is_directory ? "DIRECTORY\n" : "FILE\n");

    printf("  Direct blocks: ");
    int has_direct = 0;
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] != FS_INVALID_BLOCK) {
            printf("#%u ", inode.direct_blocks[i]);
            has_direct = 1;
        }
    }
    if (!has_direct) printf("none");
    printf("\n");

    if (inode.indirect_block != FS_INVALID_BLOCK) {
        printf("  Indirect block: %u -> ", inode.indirect_block);

        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block((int)inode.indirect_block, indirect_blocks);

        const int count = BLOCK_SIZE / (int)sizeof(uint32_t);
        int has_indirect = 0;
        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] != FS_INVALID_BLOCK) {
                printf("%u ", indirect_blocks[i]);
                has_indirect = 1;
            }
        }
        if (!has_indirect) printf("none");
        printf("\n");
    }

    return 0;
}

int fs_import(const char* src, const char* dest)
{
    // Import a host file into VFS as a regular file.
    FILE* src_file = fopen(src, "rb");
    if (!src_file) {
        return 1;
    }

    // Determine host file size
    fseek(src_file, 0, SEEK_END);
    long file_size = ftell(src_file);
    fseek(src_file, 0, SEEK_SET);

    if (file_size <= 0) {
        printf("EMPTY OR INVALID FILE\n");
        fclose(src_file);
        return 1;
    }

    if (file_size > MAX_FILE_SIZE) {
        printf("FILE TOO LARGE (max %lu bytes)\n", (unsigned long)MAX_FILE_SIZE);
        fclose(src_file);
        return 1;
    }

    // Basic capacity check (rough, based on blocks)
    if ((file_size + (BLOCK_SIZE - 1)) / BLOCK_SIZE > (long)get_amount_of_available_blocks()) {
        printf("NOT ENOUGH SPACE IN FILESYSTEM\n");
        fclose(src_file);
        return 1;
    }

    // Read host file into memory
    void* buffer = malloc((size_t)file_size);
    if (!buffer) {
        printf("MEMORY ERROR\n");
        fclose(src_file);
        return 1;
    }

    fread(buffer, 1, (size_t)file_size, src_file);
    fclose(src_file);

    dest = complete_path((char*)dest);

    // Destination inside VFS must not already exist
    if (path_exists(dest)) {
        free(buffer);
        return 2;
    }

    // Resolve destination parent directory and filename
    char parent_path[MAX_PATH_LEN];
    char vfs_name[MAX_FILENAME_LEN];

    if (!split_path(dest, parent_path, vfs_name)) {
        free(buffer);
        return 2;
    }

    const int parent_inode = find_inode_by_path(parent_path);
    if (parent_inode < 0 || !is_directory(parent_inode)) {
        free(buffer);
        return 2;
    }

    // Create destination file inode and write data
    const int new_inode = create_file(parent_inode, vfs_name, false);
    if (new_inode < 0) {
        printf("FAILED TO CREATE\n");
        free(buffer);
        return 2;
    }

    const int written = write_inode_data(new_inode, buffer, (int)file_size);
    free(buffer);

    if (written != (int)file_size) {
        printf("WRITE FAILED %d != %ld\n", written, file_size);
        return 2;
    }

    return 0;
}

int fs_export(const char* src, const char* dest)
{
    // Export a VFS file to the host filesystem.
    src = complete_path((char*)src);

    const int inode_id = find_inode_by_path(src);
    if (inode_id < 0) return 1;
    if (is_directory(inode_id)) return 1;

    void* buffer = malloc(MAX_FILE_SIZE);
    if (!buffer) return 2;

    const int bytes_read = read_inode_data(inode_id, buffer);
    if (bytes_read <= 0) {
        free(buffer);
        return 2;
    }

    FILE* dest_file = fopen(dest, "wb");
    if (!dest_file) {
        free(buffer);
        return 2;
    }

    const size_t written = fwrite(buffer, 1, (size_t)bytes_read, dest_file);
    fclose(dest_file);
    free(buffer);

    return (written == (size_t)bytes_read) ? 0 : 2;
}

int fs_load_script(const char* filename)
{
    // Load a host file containing one command per line and execute sequentially.
    FILE* f = fopen(filename, "rb");
    if (!f) return 1;

    // Prevent recursive load() calls to avoid infinite recursion.
    static int load_depth = 0;
    if (load_depth > 0) { fclose(f); return 1; }
    load_depth++;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // Strip CR/LF
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines
        if (line[0] == '\0') continue;

        // Disallow nested 'load' for safety and predictable behavior
        char cmd[64];
        if (sscanf(line, "%63s", cmd) == 1 && strcmp(cmd, "load") == 0) {
            continue;
        }

        execute_command(line);
    }

    fclose(f);
    load_depth--;
    return 0;
}

int fs_format_cmd(const int size)
{
    if (is_mounted()) {
        fs_unmount();
    }

    int res = fs_format(size, file_name);
    if (res != 0) {
        return res;
    }

    if (!fs_mount(file_name)) {
        printf("Failed to mount after format\n");
        return 1;
    }

    metadata_init();
    current_path = "/";

    return 0;
}

void fs_stat()
{
    if (!is_mounted()) {
        printf("Filesystem not mounted\n");
        return;
    }

    const struct superblock_disk* sb = fs_get_superblock_disk();
    if (!sb) {
        printf("Failed to get superblock\n");
        return;
    }

    uint32_t free_blocks_count = get_amount_of_available_blocks();
    uint32_t free_inodes_count = get_amount_of_available_inodes();

    uint32_t used_blocks = sb->total_blocks - free_blocks_count;
    uint32_t used_inodes = sb->total_inodes - free_inodes_count;

    uint32_t dir_count = 0;
    const uint8_t* inode_bm = fs_get_inode_bitmap();

    for (uint32_t i = 0; i < sb->total_inodes; i++) {
        if (inode_bm[i / 8] & (1 << (i % 8))) {
            struct pseudo_inode inode;
            read_inode(i, &inode);
            if (inode.is_directory) {
                dir_count++;
            }
        }
    }

    uint64_t total_size = (uint64_t)sb->total_blocks * sb->block_size;
    double size_mb = total_size / (1024.0 * 1024.0);

    printf("=== Filesystem Statistics ===\n");
    printf("Total size:        %.2f MB (%lu bytes)\n", size_mb, total_size);
    printf("Block size:        %u bytes\n", sb->block_size);
    printf("\n");
    printf("Blocks:\n");
    printf("  Total:           %u\n", sb->total_blocks);
    printf("  Used:            %u (%.2f%%)\n", used_blocks,
           (used_blocks * 100.0) / sb->total_blocks);
    printf("  Free:            %u (%.2f%%)\n", free_blocks_count,
           (free_blocks_count * 100.0) / sb->total_blocks);
    printf("\n");
    printf("Inodes:\n");
    printf("  Total:           %u\n", sb->total_inodes);
    printf("  Used:            %u (%.2f%%)\n", used_inodes,
           (used_inodes * 100.0) / sb->total_inodes);
    printf("  Free:            %u (%.2f%%)\n", free_inodes_count,
           (free_inodes_count * 100.0) / sb->total_inodes);
    printf("\n");
    printf("Directories:       %u\n", dir_count);
    printf("Files:             %u\n", used_inodes - dir_count);
}

char* complete_path(char* path)
{
    // Convert relative paths to absolute by prefixing current_path.
    if (!path || path[0] == '\0') return path;

    if (path[0] != '/')
    {
        const size_t len = strlen(current_path) + strlen(path) + 2;
        char* joined = (char*)malloc(len);
        if (!joined) {
            printf("malloc failed\n");
            return path;
        }

        // Avoid double slashes when current_path is root
        if (strlen(current_path) < 2) {
            snprintf(joined, len, "%s%s", current_path, path);
        } else {
            snprintf(joined, len, "%s/%s", current_path, path);
        }

        path = strdup(joined);
        free(joined);
    }

    return path;
}

int fs_xcp(char* s1, char* s2, char* s3)
{
    // Create s3 as concatenation of s1 and s2 (within VFS).
    s1 = complete_path(s1);
    s2 = complete_path(s2);
    s3 = complete_path(s3);

    const int s1_node = find_inode_by_path(s1);
    const int s2_node = find_inode_by_path(s2);
    if (s1_node < 0 || s2_node < 0) return 1;
    if (is_directory(s1_node) || is_directory(s2_node)) return 1;

    // Validate destination: parent directory must exist and s3 must not exist
    char s3_parent[MAX_PATH_LEN], s3_name[MAX_FILENAME_LEN];
    if (!split_path(s3, s3_parent, s3_name)) return 2;

    const int s3_parent_node = find_inode_by_path(s3_parent);
    if (s3_parent_node < 0 || !is_directory(s3_parent_node)) return 2;
    if (path_exists(s3)) return 2;

    // Read both sources into memory and build output buffer
    void* buf1 = malloc(MAX_FILE_SIZE);
    void* buf2 = malloc(MAX_FILE_SIZE);
    void* out  = malloc(MAX_FILE_SIZE);
    if (!buf1 || !buf2 || !out) {
        free(buf1); free(buf2); free(out);
        return 4;
    }

    const int n1 = read_inode_data(s1_node, buf1);
    const int n2 = read_inode_data(s2_node, buf2);
    if (n1 < 0 || n2 < 0) {
        free(buf1); free(buf2); free(out);
        return 1;
    }

    // Enforce max file size for the destination
    if ((long long)n1 + (long long)n2 > (long long)MAX_FILE_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    // Rough capacity check
    if ((long long)n1 + (long long)n2 > (long long)get_amount_of_available_blocks() * (long long)BLOCK_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    memcpy(out, buf1, (size_t)n1);
    memcpy((char*)out + n1, buf2, (size_t)n2);

    // Create destination file and write concatenated data
    const int new_inode_id = create_file(s3_parent_node, s3_name, false);
    if (new_inode_id < 0) {
        free(buf1); free(buf2); free(out);
        return 4;
    }

    const int written = write_inode_data(new_inode_id, out, n1 + n2);

    free(buf1); free(buf2); free(out);
    return (written == n1 + n2) ? 0 : 4;
}

int fs_add(char* s1, char* s2)
{
    // Append contents of s2 to the end of s1 (within VFS).
    s1 = complete_path(s1);
    s2 = complete_path(s2);

    const int s1_node = find_inode_by_path(s1);
    const int s2_node = find_inode_by_path(s2);
    if (s1_node < 0 || s2_node < 0) return 1;
    if (is_directory(s1_node) || is_directory(s2_node)) return 1;

    void* buf1 = malloc(MAX_FILE_SIZE);
    void* buf2 = malloc(MAX_FILE_SIZE);
    void* out  = malloc(MAX_FILE_SIZE);
    if (!buf1 || !buf2 || !out) {
        free(buf1); free(buf2); free(out);
        return 4;
    }

    // Read both files and rewrite s1 with the concatenated buffer
    const int n1 = read_inode_data(s1_node, buf1);
    const int n2 = read_inode_data(s2_node, buf2);
    if (n1 < 0 || n2 < 0) {
        free(buf1); free(buf2); free(out);
        return 1;
    }

    if ((long long)n1 + (long long)n2 > (long long)MAX_FILE_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    // Rough capacity check for additional data only
    if ((long long)n2 > (long long)get_amount_of_available_blocks() * (long long)BLOCK_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    memcpy(out, buf1, (size_t)n1);
    memcpy((char*)out + n1, buf2, (size_t)n2);

    const int written = write_inode_data(s1_node, out, n1 + n2);

    free(buf1); free(buf2); free(out);
    return (written == n1 + n2) ? 0 : 4;
}
