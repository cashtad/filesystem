#include "logic_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ================================================================
// 1️⃣  Вспомогательные функции
// ================================================================

/**
 * Проверяет, пуста ли директория (только "." и ".." допускаются).
 */
static bool is_directory_empty(int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == 0) continue;
        read_block(inode.direct_blocks[i], buffer);

        int items = BLOCK_SIZE / sizeof(struct directory_item);
        for (int j = 0; j < items; j++) {
            if (buffer[j].inode_id != 0) return false;
        }
    }

    // Проверяем single indirect block (если есть)
    if (inode.indirect_block != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect_block, indirect_blocks);
        int count = BLOCK_SIZE / sizeof(uint32_t);

        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] != 0) {
                read_block(indirect_blocks[i], buffer);
                int items = BLOCK_SIZE / sizeof(struct directory_item);
                for (int j = 0; j < items; j++) {
                    if (buffer[j].inode_id != 0) return false;
                }
            }
        }
    }

    return true;
}

/**
 * Загружает inode и проверяет тип.
 */
bool is_directory(int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);
    return inode.is_directory != 0;
}

/**
 * Делит путь на имя родителя и последнего элемента.
 * Например: "/dir1/dir2/file" → parent="/dir1/dir2", name="file"
 */
static bool split_path(const char* path, char* parent, char* name) {
    if (!path || strlen(path) == 0) return false;
    char temp[MAX_PATH_LEN];
    strncpy(temp, path, MAX_PATH_LEN);

    char* last_slash = strrchr(temp, '/');
    if (!last_slash) return false;

    *last_slash = '\0';
    strcpy(name, last_slash + 1);
    if (strlen(temp) == 0)
        strcpy(parent, "/");
    else
        strcpy(parent, temp);

    return true;
}

// ================================================================
// 2️⃣  Работа с директориями
// ================================================================

int find_item_in_directory(int parent_inode, const char* name) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode);

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    int items_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    // Проверка прямых блоков
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == 0) continue;
        read_block(inode.direct_blocks[i], buffer);

        for (int j = 0; j < items_per_block; j++) {
            if (strcmp(buffer[j].name, name) == 0)
                return buffer[j].inode_id;
        }
    }

    // Проверка через один уровень индирекции
    if (inode.indirect_block != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect_block, indirect_blocks);

        int count = BLOCK_SIZE / sizeof(uint32_t);
        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] == 0) continue;
            read_block(indirect_blocks[i], buffer);

            for (int j = 0; j < items_per_block; j++) {
                if (strcmp(buffer[j].name, name) == 0)
                    return buffer[j].inode_id;
            }
        }
    }

    return -1;
}

bool add_directory_item(int parent_inode, const char* name, int child_inode) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode);

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    int items_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    // Пробуем вставить в прямые блоки
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == 0) {
            inode.direct_blocks[i] = allocate_free_block();
            write_inode(parent_inode, &inode);
            memset(buffer, 0, sizeof(buffer));
        } else {
            read_block(inode.direct_blocks[i], buffer);
        }

        for (int j = 0; j < items_per_block; j++) {
            if (buffer[j].inode_id == 0) {
                strcpy(buffer[j].name, name);
                buffer[j].inode_id = child_inode;
                write_block(inode.direct_blocks[i], buffer);
                return true;
            }
        }
    }

    // Пробуем вставить через индиректный блок
    if (inode.indirect_block == 0) {
        inode.indirect_block = allocate_free_block();
        write_inode(parent_inode, &inode);
        uint32_t zeroes[BLOCK_SIZE / sizeof(uint32_t)] = {0};
        write_block(inode.indirect_block, zeroes);
    }

    uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
    read_block(inode.indirect_block, indirect_blocks);
    int count = BLOCK_SIZE / sizeof(uint32_t);

    for (int i = 0; i < count; i++) {
        if (indirect_blocks[i] == 0) {
            indirect_blocks[i] = allocate_free_block();
            write_block(inode.indirect_block, indirect_blocks);
            memset(buffer, 0, sizeof(buffer));
        } else {
            read_block(indirect_blocks[i], buffer);
        }

        for (int j = 0; j < items_per_block; j++) {
            if (buffer[j].inode_id == 0) {
                strcpy(buffer[j].name, name);
                buffer[j].inode_id = child_inode;
                write_block(indirect_blocks[i], buffer);
                return true;
            }
        }
    }

    return false;
}

bool remove_directory_item(int parent_inode, const char* name) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode);

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    int items_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    // Прямые блоки
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == 0) continue;
        read_block(inode.direct_blocks[i], buffer);

        for (int j = 0; j < items_per_block; j++) {
            if (strcmp(buffer[j].name, name) == 0) {
                buffer[j].inode_id = 0;
                buffer[j].name[0] = '\0';
                write_block(inode.direct_blocks[i], buffer);
                return true;
            }
        }
    }

    // Индиректные блоки
    if (inode.indirect_block != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect_block, indirect_blocks);
        int count = BLOCK_SIZE / sizeof(uint32_t);

        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] == 0) continue;
            read_block(indirect_blocks[i], buffer);

            for (int j = 0; j < items_per_block; j++) {
                if (strcmp(buffer[j].name, name) == 0) {
                    buffer[j].inode_id = 0;
                    buffer[j].name[0] = '\0';
                    write_block(indirect_blocks[i], buffer);
                    return true;
                }
            }
        }
    }

    return false;
}

void list_directory(int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    int items_per_block = BLOCK_SIZE / sizeof(struct directory_item);

    printf("Contents of directory (inode %d):\n", inode_id);

    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == 0) continue;
        read_block(inode.direct_blocks[i], buffer);
        for (int j = 0; j < items_per_block; j++) {
            if (buffer[j].inode_id != 0)
                printf("  %s (inode %d)\n", buffer[j].name, buffer[j].inode_id);
        }
    }

    if (inode.indirect_block != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect_block, indirect_blocks);
        int count = BLOCK_SIZE / sizeof(uint32_t);

        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] == 0) continue;
            read_block(indirect_blocks[i], buffer);
            for (int j = 0; j < items_per_block; j++) {
                if (buffer[j].inode_id != 0)
                    printf("  %s (inode %d)\n", buffer[j].name, buffer[j].inode_id);
            }
        }
    }
}

// ================================================================
// 3️⃣  Работа с путями и созданием файлов
// ================================================================

int find_inode_by_path(const char* path) {
    if (strcmp(path, "/") == 0) return 0; // корень всегда inode 0

    char temp[MAX_PATH_LEN];
    strncpy(temp, path, MAX_PATH_LEN);
    char* token = strtok(temp, "/");
    int current_inode = 0;

    while (token != NULL) {
        current_inode = find_item_in_directory(current_inode, token);
        if (current_inode < 0) return -1;
        token = strtok(NULL, "/");
    }

    return current_inode;
}

bool path_exists(const char* path) {
    return find_inode_by_path(path) >= 0;
}

int create_file(int parent_inode, const char* name, bool isDirectory) {
    int inode_id = allocate_free_inode();
    if (inode_id < 0) return -1;

    struct pseudo_inode inode = {0};
    inode.id = inode_id;
    inode.is_directory = isDirectory;
    inode.file_size = 0;
    inode.amount_of_links = 1;

    write_inode(inode_id, &inode);
    add_directory_item(parent_inode, name, inode_id);
    return inode_id;
}

void delete_file(int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (inode.is_directory && !is_directory_empty(inode_id)) {
        printf("Cannot delete non-empty directory (inode %d)\n", inode_id);
        return;
    }

    // Освобождаем все блоки
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] != 0)
            free_block(inode.direct_blocks[i]);
    }

    if (inode.indirect_block != 0) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block(inode.indirect_block, indirect_blocks);
        int count = BLOCK_SIZE / sizeof(uint32_t);

        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] != 0)
                free_block(indirect_blocks[i]);
        }

        free_block(inode.indirect_block);
    }

    free_inode(inode_id);
}

void logic_init(void) {
    if (!path_exists("/")) {
        int root_inode = allocate_free_inode();
        struct pseudo_inode root = {0};
        root.id = root_inode;
        root.is_directory = 1;
        root.file_size = 0;
        root.amount_of_links = 1;
        write_inode(root_inode, &root);
        printf("Root directory initialized at inode %d\n", root_inode);
    }
}
