#include "shell_layer.h"
#include "../logic/logic_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* current_path;

// ================================================================
// 🔹 Shell main loop
// ================================================================

void run_shell()
{
    printf("=== Virtual File System Shell ===\n");
    printf("Type 'help' for list of supported commands.\n");

    fs_mount(CURRENT_FS_FILENAME);

    metadata_init();

    current_path = "/";

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
    char cmd[64], arg1[256], arg2[256];
    int args = sscanf(input, "%63s %255s %255s", cmd, arg1, arg2);

    if (args < 1) return;

    // ================================================================
    // 1) cp s1 s2
    // ================================================================
    if (strcmp(cmd, "cp") == 0)
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
        else if (res == 3) printf("UNKNOW ERROR\n");
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
        printf("%s\n", current_path);

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

// ДЕЛАЕТ ССЫЛКУ НА ФАЙЛ
int fs_copy(char* src, char* dest) {
    // char *parent_path[MAX_PATH_LEN], *src_name[FILENAME_MAX];
    // if (!split_path(src, parent_path, src_name)) return 1;
    //
    // const int src_node = find_inode_by_path(src);
    // const int dest_node = find_inode_by_path(dest);
    //
    // if (src_node < 0) return 1;
    // if (dest_node < 0) return 2;
    //
    // if (add_directory_item(dest_node, src_name, dest_node)) return 0;
    //
    // return 3;
}

int fs_move(char* src, char* dest) {

    src = complete_path(src);
    dest = complete_path(dest);

    printf("%s\n", src);
    printf("%s\n", dest);

    char *parent_path[MAX_PATH_LEN], *src_name[FILENAME_MAX];
    split_path(src, parent_path, src_name);

    printf("%s\n", src);
    printf("%s\n", parent_path);
    printf("%s\n", src_name);

    const int src_node = find_inode_by_path(src);
    const int dest_node = find_inode_by_path(dest);

    printf("%d %d\n", src_node, dest_node);

    if (src_node < 0) return 1;
    if (dest_node < 0) return 2;

    if (add_directory_item(dest_node, src_name, src_node)
        & remove_directory_item(find_inode_by_path(parent_path), src_name)) return 0;
    return 3;
}

int fs_remove(char* path) {

    // СЕЙЧАС УДАЛЯЕТ КАК ФАЙЛЫ, ТАК И ПАПКИ. МБ СТОИТ ИСПРАВИТЬ
    path = complete_path(path);

    const int res = delete_file(path);

    return res;
}

int fs_mkdir(char* path) {

    path = complete_path(path);

    if (path_exists(path)) return 2; // если папка уже существует

    char *parent_path[MAX_PATH_LEN], *dir_name[FILENAME_MAX];
    split_path(path, parent_path, dir_name); // разбиваем путь на предка (папку) и название файла

    const int parent_node = find_inode_by_path(parent_path); // находим айди предка

    if (create_file(parent_node, dir_name, true) > 0) return 0; // создаем в предке новую папку

    return 1;
}

int fs_rmdir(char* path) {

    path = complete_path(path);

    const int node_id = find_inode_by_path(path);

    if (node_id < 0) return 1;

    if (!is_directory_empty(node_id)) return 2;

    return delete_file(path);
}

int fs_ls(char* path) {
    if (strcmp(path, ".") == 0)
    {
        path = current_path;
    } else
    {
        path = complete_path(path);
    }
    const int node_id = find_inode_by_path(path);
    if (node_id < 0) return 1;
    if (is_directory(node_id)) {
        list_directory(node_id);
        return 0;
    }
    return 1;
}

int fs_cat(char* path)
{
}

int fs_cd(char* path) {

    path = complete_path(path);

    if (path_exists(path))
    {
        current_path = strdup(path);
        return 0;
    }

    return 1;
}

void fs_pwd(void* buffer)
{
    buffer = current_path;
}

int fs_info(char* path)
{
}

int fs_import(const char* src, const char* dest)
{
}

int fs_export(const char* src, const char* dest)
{
}

int fs_load_script(const char* filename)
{
}

int fs_format_cmd(const int size)
{ fs_format(size);
    current_path = "/";
}

void fs_stat()
{
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