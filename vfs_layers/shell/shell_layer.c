#include "shell_layer.h"
#include "../logic/logic_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* current_path;
static char* file_name;


int init()
{
    int res = fs_mount(file_name);
    if (!is_mounted())
    {
        printf("Failed to mount file system.\nTry to format the fs with 'format <size MB>'\n");
    } else
    {
        metadata_init();
        current_path = "/";
    }
    return res;
}

// ================================================================
// 🔹 Shell main loop
// ================================================================

void run_shell(const char *filesystem_name)
{
    printf("=== Virtual File System Shell ===\n");
    printf("Type 'help' for list of supported commands.\n");
    file_name = strdup(filesystem_name);

    init();

    char input[1024];
    while (1)
    {
        printf("vfs> ");
        if (!fgets(input, sizeof(input), stdin)) break;
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
    char cmd[64], arg1[256], arg2[256], arg3[256];
    int args = sscanf(input, "%63s %255s %255s %255s", cmd, arg1, arg2, arg3);

    if (args < 1) return;

    if (!is_mounted())
    {
        if (strcmp(cmd, "format") == 0)
        {
            if (args < 2)
            {
                printf("Usage: format <sizeMB>\n");
                return;
            }
            int size = strtol(arg1, 0, 0);
            int res = fs_format_cmd(size);
            if (res == 0) printf("OK\n");
            else printf("CANNOT CREATE FILE\n");
        }
        else
        {
            printf("FS not mounted! You need to mount it with the command 'format <sizeMB>'\n");
        }
    } else
    {

    // ================================================================
    // NEW) xcp s1 s2 s3
    // ================================================================
    if (strcmp(cmd, "xcp") == 0)
    {
        if (args < 4)
        {
            printf("Usage: xcp s1 s2 s3\n");
            return;
        }
        int res = fs_xcp(arg1, arg2, arg3);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else if (res == 3) printf("FILE TOO LARGE\n");
        else printf("UNKNOW ERROR\n");
    }

    // ================================================================
    // NEW) add s1 s2
    // ================================================================
    else if (strcmp(cmd, "add") == 0)
    {
        if (args < 3)
        {
            printf("Usage: add s1 s2\n");
            return;
        }
        int res = fs_add(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 3) printf("FILE TOO LARGE\n");
        else printf("UNKNOW ERROR\n");
    }

    // ================================================================
    // 1) cp s1 s2
    // ================================================================
    else if (strcmp(cmd, "cp") == 0)
    {
        if (args < 3)
        {
            printf("Usage: cp s1 s2\n");
            return;
        }
        int res = fs_copy(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
        else printf("UNKNOW ERROR\n");
    }

    // ================================================================
    // 2) mv s1 s2
    // ================================================================
    else if (strcmp(cmd, "mv") == 0)
    {
        if (args < 3)
        {
            printf("Usage: mv s1 s2\n");
            return;
        }
        int res = fs_move(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 3) rm s1
    // ================================================================
    else if (strcmp(cmd, "rm") == 0)
    {
        if (args < 2)
        {
            printf("Usage: rm s1\n");
            return;
        }
        int res = fs_remove(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 2) printf("CANNOT REMOVE DIRECTORY\n");
        else printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 4) mkdir a1
    // ================================================================
    else if (strcmp(cmd, "mkdir") == 0)
    {
        if (args < 2)
        {
            printf("Usage: mkdir a1\n");
            return;
        }
        int res = fs_mkdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("PATH NOT FOUND\n");
        else if (res == 2) printf("EXIST\n");
    }

    // ================================================================
    // 5) rmdir a1
    // ================================================================
    else if (strcmp(cmd, "rmdir") == 0)
    {
        if (args < 2)
        {
            printf("Usage: rmdir a1\n");
            return;
        }
        int res = fs_rmdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("NOT EMPTY\n");
    }

    // ================================================================
    // 6) ls [a1]
    // ================================================================
    else if (strcmp(cmd, "ls") == 0)
    {
        if (args == 1) fs_ls(".");
        else if (args >= 2)
        {
            if (fs_ls(arg1) != 0)
            {
                printf("PATH NOT FOUND\n");
            }
        }
    }

    // ================================================================
    // 7) cat s1
    // ================================================================
    else if (strcmp(cmd, "cat") == 0)
    {
        if (args < 2)
        {
            printf("Usage: cat s1\n");
            return;
        }
        int res = fs_cat(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 8) cd a1
    // ================================================================
    else if (strcmp(cmd, "cd") == 0)
    {
        if (args < 2)
        {
            printf("Usage: cd a1\n");
            return;
        }
        int res = fs_cd(arg1);
        if (res == 0) printf("OK\n");
        else printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 9) pwd
    // ================================================================
    else if (strcmp(cmd, "pwd") == 0)
    {
        // char path[MAX_PATH_LEN];
        // fs_pwd(path);
        printf("Current path: %s\n", current_path);
    }

    // ================================================================
    // 10) info s1
    // ================================================================
    else if (strcmp(cmd, "info") == 0)
    {
        if (args < 2)
        {
            printf("Usage: info s1\n");
            return;
        }
        int res = fs_info(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 11) incp s1 s2
    // ================================================================
    else if (strcmp(cmd, "incp") == 0)
    {
        if (args < 3)
        {
            printf("Usage: incp s1 s2\n");
            return;
        }
        int res = fs_import(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 12) outcp s1 s2
    // ================================================================
    else if (strcmp(cmd, "outcp") == 0)
    {
        if (args < 3)
        {
            printf("Usage: outcp s1 s2\n");
            return;
        }
        int res = fs_export(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 13) load s1
    // ================================================================
    else if (strcmp(cmd, "load") == 0)
    {
        if (args < 2)
        {
            printf("Usage: load s1\n");
            return;
        }
        int res = fs_load_script(arg1);
        if (res == 0) printf("OK\n");
        else printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 14) format <sizeMB>
    // ================================================================
    else if (strcmp(cmd, "format") == 0)
    {
        if (args < 2)
        {
            printf("Usage: format <sizeMB>\n");
            return;
        }
        int size = strtol(arg1, 0, 0);
        int res = fs_format_cmd(size);
        if (res == 0) printf("OK\n");
        else printf("CANNOT CREATE FILE\n");
    }

    // ================================================================
    // 15) exit
    // ================================================================
    else if (strcmp(cmd, "exit") == 0)
    {
        fs_unmount();
        printf("Exiting...\n");
        exit(0);
    }

    // ================================================================
    // 16) statfs
    // ================================================================
    else if (strcmp(cmd, "statfs") == 0)
    {
        fs_stat();
    }

    // ================================================================
    // Unknown command
    // ================================================================
    else
    {
        printf("Unknown command\n");
    }
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

    if (add_directory_item(dest_parent_node, dest_name, src_node)
        & remove_directory_item(find_inode_by_path(parent_path), src_name)) return 0;
    return 3;
}

int fs_remove(char* path) {

    // СЕЙЧАС УДАЛЯЕТ КАК ФАЙЛЫ, ТАК И ПАПКИ. МБ СТОИТ ИСПРАВИТЬ
    path = complete_path(path);

    const int node_id = find_inode_by_path(path);

    if (node_id < 0) return 1;

    if (is_directory(node_id)) return 2;

    const int res = delete_file(path);

    return res;
}

int fs_mkdir(char* path) {

    path = complete_path(path);

    if (path_exists(path)) return 2; // если папка уже существует

    char parent_path[MAX_PATH_LEN];
    char dir_name[MAX_FILENAME_LEN];

    if (!split_path(path, parent_path, dir_name)) return 1;

    const int parent_node = find_inode_by_path(parent_path); // находим айди предка
    if (parent_node < 0) return 1;

    int new_inode = create_file(parent_node, dir_name, true);
    if (new_inode >= 0) return 0; // успешно создана директория

    return 1;
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

    printf("cat %s\n", path);
    path = complete_path(path);

    int inode_id = find_inode_by_path(path);

    if (inode_id < 0) return 1;

    if (is_directory(inode_id)) return 1;

    char* buffer = malloc(MAX_FILE_SIZE);
    if (!buffer) {
        printf("MEMORY ERROR\n");
        return 1;
    }
    int read_bytes = read_inode_data(inode_id, buffer);

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

/**
 * @brief Выводит информацию о файле или директории по пути.
 * Формат:
 *   NAME – SIZE – i-node NUMBER – direct и indirect блоки
 * Пример:
 *   ahoj.txt – 1800 B – i-node 7 – přímé odkazy 101, 102
 */
int fs_info(char* path)
{
    path = complete_path(path);
    // 1️⃣ Проверяем, существует ли путь
    int inode_id = find_inode_by_path(path);
    if (inode_id < 0) {
        printf("FILE NOT FOUND\n");
        return 1;
    }

    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    // 2️⃣ Вывод имени (последнего компонента пути)
    const char* name = strrchr(path, '/');
    if (!name) name = path;
    else name++; // пропустить '/'

    if (strlen(name) == 0) name = "/"; // если это корень

    int file_size = (int) inode.file_size;

    // 3️⃣ Основная информация
    printf("%s - %d B - i-node %d - ", name, file_size, inode_id);

    if (inode.is_directory)
        printf("DIRECTORY\n");
    else
        printf("FILE\n");

    // 4️⃣ Список прямых блоков
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

    // 5️⃣ Список косвенных блоков (если есть)
    if (inode.indirect_block != FS_INVALID_BLOCK) {
        printf("  Undirect links: %u → ", inode.indirect_block);
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block((int)inode.indirect_block, indirect_blocks);
        int count = BLOCK_SIZE / sizeof(uint32_t);
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
    FILE* src_file = fopen(src, "rb");
    if (!src_file) {
        return 1;
    }

    // 2 Получаем размер файла
    fseek(src_file, 0, SEEK_END);
    long file_size = ftell(src_file);
    fseek(src_file, 0, SEEK_SET);

    if (file_size <= 0) {
        printf("EMPTY OR INVALID FILE\n");
        fclose(src_file);
        return 1;
    }

    if (file_size > MAX_FILE_SIZE) {
        printf("FILE TOO LARGE (max %lu bytes)\n", MAX_FILE_SIZE);
        fclose(src_file);
        return 1;
    }

    if (file_size / BLOCK_SIZE > get_amount_of_available_blocks()) {
        printf("NOT ENOUGH SPACE IN FILESYSTEM\n");
        fclose(src_file);
        return 1;
    }

    // 3 Считываем файл в буфер
    void* buffer = malloc(file_size);
    if (!buffer) {
        printf("MEMORY ERROR\n");
        fclose(src_file);
        return 1;
    }

    fread(buffer, 1, file_size, src_file);
    fclose(src_file);

    dest = complete_path(dest);

    if (path_exists(dest)) {
        free(buffer);
        return 2;
    }

    char parent_path[MAX_PATH_LEN];
    char file_name[MAX_FILENAME_LEN];

    if (!split_path(dest, parent_path, file_name)) {
        free(buffer);
        return 2;
    }

    int parent_inode = find_inode_by_path(parent_path);

    if (parent_inode < 0 && !is_directory(parent_inode)) {
        free(buffer);
        return 2;
    }

    // 7️⃣ Создаём новый файл во внутренней FS
    int new_inode = create_file(parent_inode, file_name, false);
    if (new_inode < 0) {
        printf("FAILED TO CREATE\n");
        free(buffer);
        return 2;
    }

    // 8️⃣ Записываем содержимое в VFS
    int written = write_inode_data(new_inode, buffer, file_size);
    if (written != file_size) {
        printf("WRITE FAILED %d != %lu\n", written , file_size);
        free(buffer);
        return 2;
    }

    // 9️⃣ Всё успешно
    free(buffer);
    return 0;

}

/**
 * @brief Exports a file from the virtual filesystem to the host system.
 *
 * Command syntax: outcp <source_path_vfs> <destination_path_host>
 *
 * Example:
 *    outcp "/docs/test.txt" "output/test_copy.txt"
 *
 * Return codes:
 *   0 — OK
 *   1 — FILE NOT FOUND (внутри VFS)
 *   2 — WRITE ERROR / INVALID PATH (нельзя создать на хосте)
 */
int fs_export(const char* src, const char* dest)
{
    src = complete_path(src);
    // 1️⃣ Находим inode исходного файла во внутренней FS
    int inode_id = find_inode_by_path(src);
    if (inode_id < 0) {
        printf("FILE NOT FOUND\n");
        return 1;
    }

    // 2️⃣ Проверяем, что это именно файл, а не директория
    if (is_directory(inode_id)) {
        printf("CANNOT EXPORT DIRECTORY\n");
        return 1;
    }

    // 3️⃣ Читаем содержимое файла из FS
    void* buffer = malloc(MAX_FILE_SIZE);
    if (!buffer) {
        printf("MEMORY ERROR\n");
        return 2;
    }

    int bytes_read = read_inode_data(inode_id, buffer);
    if (bytes_read <= 0) {
        printf("READ ERROR\n");
        free(buffer);
        return 2;
    }

    // 4️⃣ Открываем файл на хосте для записи
    FILE* dest_file = fopen(dest, "wb");
    if (!dest_file) {
        printf("PATH NOT FOUND OR CANNOT CREATE FILE\n");
        free(buffer);
        return 2;
    }

    // 5️⃣ Записываем содержимое
    size_t written = fwrite(buffer, 1, bytes_read, dest_file);
    fclose(dest_file);
    free(buffer);

    if (written != (size_t)bytes_read) {
        printf("WRITE ERROR (%zu != %d)\n", written, bytes_read);
        return 2;
    }

    printf("OK\n");
    return 0;
}

int fs_load_script(const char* filename)
{
    // читаем файл С ХОСТА (pevný диск), 1 команда/строка
    FILE* f = fopen(filename, "rb");
    if (!f) return 1; // FILE NOT FOUND (нет источника)

    // защита от рекурсивных/вложенных load
    static int load_depth = 0;
    if (load_depth > 0) { fclose(f); return 1; }
    load_depth++;

    char line[1024];
    while (fgets(line, sizeof(line), f))
    {
        // убрать \r\n
        line[strcspn(line, "\r\n")] = 0;

        // пропустить пустые строки
        if (line[0] == '\0') continue;

        // запрет "load ..." внутри скрипта (простая защита)
        char cmd[64];
        if (sscanf(line, "%63s", cmd) == 1 && strcmp(cmd, "load") == 0) {
            // по ТЗ нет отдельного текста/кода ошибки — просто считаем как "неуспех"
            continue;
        }

        execute_command(line);
    }

    fclose(f);
    load_depth--;
    return 0; // OK
}

int fs_format_cmd(const int size)
{
    // Размонтируем перед форматированием
    if (is_mounted()) {
        fs_unmount();
    }

    int res = fs_format(size, file_name);
    if (res != 0) {
        return res;
    }

    // После успешного форматирования монтируем заново
    if (!fs_mount(file_name)) {
        printf("Failed to mount after format\n");
        return 1;
    }

    // Инициализируем метаданные
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

    // ИСПРАВЛЕНИЕ: используем uint32_t вместо uint16_t
    uint32_t free_blocks_count = get_amount_of_available_blocks();
    uint32_t free_inodes_count = get_amount_of_available_inodes();

    // Вычисляем занятые
    uint32_t used_blocks = sb->total_blocks - free_blocks_count;
    uint32_t used_inodes = sb->total_inodes - free_inodes_count;

    // Подсчитываем количество директорий
    uint32_t dir_count = 0;
    const uint8_t* inode_bm = fs_get_inode_bitmap();

    for (uint32_t i = 0; i < sb->total_inodes; i++) {
        // Проверяем, занят ли inode
        if (inode_bm[i / 8] & (1 << (i % 8))) {
            struct pseudo_inode inode;
            read_inode(i, &inode);
            if (inode.is_directory) {
                dir_count++;
            }
        }
    }

    // Размер ФС в байтах и МБ
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
    // если указан не полный путь
    if (path[0] != '/')
    {
        const size_t len = strlen(current_path) + strlen(path) + 2;
        char* new_path = malloc(len);
        if (!new_path)
        {
            printf("malloc failed\n");
        }
        if (strlen(current_path) < 3)
        {
            snprintf(new_path, len, "%s%s", current_path, path);
        } else
        {
            snprintf(new_path, len, "%s/%s", current_path, path);
        }
        path = strdup(new_path);
        free(new_path);
    }
    return path;
}

int fs_xcp(char* s1, char* s2, char* s3)
{
    s1 = complete_path(s1);
    s2 = complete_path(s2);
    s3 = complete_path(s3);

    const int s1_node = find_inode_by_path(s1);
    const int s2_node = find_inode_by_path(s2);

    if (s1_node < 0 || s2_node < 0) return 1;
    if (is_directory(s1_node) || is_directory(s2_node)) return 1;

    // dest parent must exist and be directory; s3 must not exist
    char s3_parent[MAX_PATH_LEN], s3_name[MAX_FILENAME_LEN];
    if (!split_path(s3, s3_parent, s3_name)) return 2;

    const int s3_parent_node = find_inode_by_path(s3_parent);
    if (s3_parent_node < 0 || !is_directory(s3_parent_node)) return 2;

    if (path_exists(s3)) return 2;

    void* buf1 = malloc(MAX_FILE_SIZE);
    void* buf2 = malloc(MAX_FILE_SIZE);
    void* out  = malloc(MAX_FILE_SIZE);
    if (!buf1 || !buf2 || !out) {
        free(buf1); free(buf2); free(out);
        return 4;
    }

    const int n1 = read_inode_data(s1_node, buf1);
    const int n2 = read_inode_data(s2_node, buf2);
    if (n1 < 0 || n2 < 0) { free(buf1); free(buf2); free(out); return 1; }

    if ((long long)n1 + (long long)n2 > (long long)MAX_FILE_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    if ((long long)n1 + (long long)n2 > (int)get_amount_of_available_blocks() * BLOCK_SIZE) {
    free(buf1); free(buf2); free(out); return 3;}

    memcpy(out, buf1, n1);
    memcpy((char*)out + n1, buf2, n2);

    const int new_inode_id = create_file(s3_parent_node, s3_name, false);
    if (new_inode_id < 0) { free(buf1); free(buf2); free(out); return 4; }

    const int written = write_inode_data(new_inode_id, out, n1 + n2);

    free(buf1); free(buf2); free(out);

    return (written == n1 + n2) ? 0 : 4;
}

int fs_add(char* s1, char* s2)
{
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

    const int n1 = read_inode_data(s1_node, buf1);
    const int n2 = read_inode_data(s2_node, buf2);
    if (n1 < 0 || n2 < 0) { free(buf1); free(buf2); free(out); return 1; }

    if ((long long)n1 + (long long)n2 > (long long)MAX_FILE_SIZE) {
        free(buf1); free(buf2); free(out);
        return 3;
    }

    if ((long long)n2 > (int)get_amount_of_available_blocks() * BLOCK_SIZE) {
        free(buf1); free(buf2); free(out); return 3;}

    memcpy(out, buf1, n1);
    memcpy((char*)out + n1, buf2, n2);

    const int written = write_inode_data(s1_node, out, n1 + n2);

    free(buf1); free(buf2); free(out);

    return (written == n1 + n2) ? 0 : 4;
}
