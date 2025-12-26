#include "logic_layer.h"


// ================================================================
// 1️⃣  Вспомогательные функции
// ================================================================

/**
 * Проверяет, пуста ли директория (все элементы inode_id == 0).
 * Директория всегда хранится в одном блоке.
 */
bool is_directory_empty(const int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode); // читаем данные ноды по ее айди

    // если не директория, то выкидываем фаталку
    if (inode.is_directory == 0) {
        printf("FATAL ERROR: inode %d is not a directory\n", inode_id);
        exit(1);
    }

    if (inode.direct_blocks[0] == FS_INVALID_BLOCK)
        return true;

    const int items = BLOCK_SIZE / sizeof(struct directory_item);
    struct directory_item buffer[items];

    read_block((int)inode.direct_blocks[0], buffer);

    for (int j = 0; j < items; j++) {
        if (buffer[j].inode_id != FS_INVALID_INODE)
            return false;
    }

    return true;
}

/**
 * Проверяет, является ли inode директорией.
 */
bool is_directory(const int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);
    return inode.is_directory != 0;
}

/**
 * Делит путь на родительский каталог и имя элемента.
 * Например: "/dir1/dir2/file" → parent="/dir1/dir2", name="file".
 */
bool split_path(const char* path, char* parent, char* name) {
    size_t name_size = MAX_FILENAME_LEN;
    if (!path || strlen(path) == 0) return false;

    char temp[MAX_PATH_LEN];
    strncpy(temp, path, MAX_PATH_LEN - 1);
    temp[MAX_PATH_LEN - 1] = '\0'; // гарантируем, что строка завершена

    char* last_slash = strrchr(temp, '/');
    if (!last_slash) return false;

    *last_slash = '\0';
    const char* filename = last_slash + 1;

    // Проверяем длину имени файла
    if (strlen(filename) >= name_size) {
        return false;
    }

    // Копируем безопасно
    strncpy(name, filename, name_size - 1);
    name[name_size - 1] = '\0'; // на всякий случай

    if (strlen(temp) == 0)
        strcpy(parent, "/");
    else
        strncpy(parent, temp, MAX_PATH_LEN - 1);

    parent[MAX_PATH_LEN - 1] = '\0'; // тоже завершаем строку

    return true;
}


// ================================================================
// 2️⃣  Работа с директориями
// ================================================================

/**
 * Ищет элемент в директории по имени.
 * Возвращает inode найденного элемента или -1, если не найден.
 */
int find_item_in_directory(const int parent_inode, const char* name) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode);

    if (!inode.is_directory) {
        printf("ERROR: inode %d is not a directory\n", parent_inode);
        return -1;
    }

    if (inode.direct_blocks[0] == FS_INVALID_BLOCK)
        return -1; // директория пуста

    const int items = BLOCK_SIZE / sizeof(struct directory_item);
    struct directory_item buffer[items];
    read_block((int)inode.direct_blocks[0], buffer);

    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id != FS_INVALID_INODE && strcmp(buffer[i].name, name) == 0)
            return (int)buffer[i].inode_id;
    }

    return -1;
}

/**
 * Добавляет элемент (файл/каталог) в директорию.
 * Возвращает true при успехе, false если каталог переполнен.
 */
bool add_directory_item(const int parent_inode, const char* name, const int child_inode) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode);

    if (!inode.is_directory) {
        printf("ERROR: inode %d is not a directory\n", parent_inode);
        return false;
    }

    if (inode.direct_blocks[0] == FS_INVALID_BLOCK) {
        inode.direct_blocks[0] = (uint32_t)allocate_free_block();
        write_inode(parent_inode, &inode);

        struct directory_item zeroes[BLOCK_SIZE / sizeof(struct directory_item)];
        memset(zeroes, 0, sizeof(zeroes));
        for (int i = 0; i < (int)(BLOCK_SIZE / sizeof(struct directory_item)); i++) {
            zeroes[i].inode_id = FS_INVALID_INODE;
        }
        write_block((int)inode.direct_blocks[0], zeroes);
    }

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    read_block((int)inode.direct_blocks[0], buffer);
    const int items = BLOCK_SIZE / sizeof(struct directory_item);

    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id == FS_INVALID_INODE) {
            strcpy(buffer[i].name, name);
            buffer[i].inode_id = (uint32_t)child_inode;
            write_block((int)inode.direct_blocks[0], buffer);
            return true;
        }
    }

    printf("ERROR: Directory (inode %d) is full\n", parent_inode);
    return false; // не осталось места
}

/**
 * Удаляет элемент из директории.
 */
bool remove_directory_item(const int parent_inode, const char* name) {
    struct pseudo_inode inode;
    read_inode(parent_inode, &inode); // загрузили инфу по айди

    if (!inode.is_directory) {
        printf("ERROR: inode %d is not a directory\n", parent_inode);
        return false;
    }

    if (inode.direct_blocks[0] == FS_INVALID_BLOCK) // если блок не выделен, значит, что каталог пуст
        return false;

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    read_block((int)inode.direct_blocks[0], buffer);
    const int items = BLOCK_SIZE / sizeof(struct directory_item);

    for (int i = 0; i < items; i++) {
        if (strcmp(buffer[i].name, name) == 0) { // если нашли ноду с таким именем, то стираем инфу
            buffer[i].inode_id = FS_INVALID_INODE;
            buffer[i].name[0] = '\0';
            write_block((int)inode.direct_blocks[0], buffer); // перезаписываем блок (без того элемента)
            return true;
        }
    }

    return false;
}

/**
 * Выводит список содержимого директории в стиле ls.
 */
void list_directory(const int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (!inode.is_directory) {
        printf("ERROR: inode %d is not a directory\n", inode_id);
        return;
    }

    if (inode.direct_blocks[0] == FS_INVALID_BLOCK) {
        printf("(empty directory)\n");
        return;
    }

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    read_block((int)inode.direct_blocks[0], buffer);
    const int items = BLOCK_SIZE / sizeof(struct directory_item);

    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id != FS_INVALID_INODE)
            printf("  %s (inode %u)\n", buffer[i].name, buffer[i].inode_id);
    }
    // for (int i = 0; i < items; i++) {
    //     if (buffer[i].inode_id != 0) {
    //         struct pseudo_inode child_inode;
    //         read_inode(buffer[i].inode_id, &child_inode);
    //
    //         // Префикс для типа: d для директории, - для файла
    //         char type = child_inode.is_directory ? 'd' : '-';
    //
    //         // Размер файла (для директорий показываем размер блока)
    //         int size = child_inode.is_directory ? BLOCK_SIZE : child_inode.file_size;
    //
    //         // Форматированный вывод: тип, размер (с выравниванием), имя
    //         printf("%c  %8d  %s\n", type, size, buffer[i].name);
    //     }
    // }
}

// ================================================================
// 3️⃣ Работа с путями и созданием файлов
// ================================================================

int find_inode_by_path(const char* path) {
    if (strcmp(path, "/") == 0) return 0; // корень — inode 0

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

/**
 * Создаёт файл или директорию и добавляет её в родительский каталог.
 */
int create_file(const int parent_inode, const char* name, const bool isDirectory) {
    const int inode_id = allocate_free_inode();
    if (inode_id < 0) return -1;

    struct pseudo_inode inode = {0};
    inode.id = inode_id;
    inode.is_directory = isDirectory;
    inode.file_size = 0;
    inode.amount_of_links = 1;

    for (int i = 0; i < 5; i++) inode.direct_blocks[i] = FS_INVALID_BLOCK;
    inode.indirect_block = FS_INVALID_BLOCK;

    if (isDirectory) {
        inode.direct_blocks[0] = (uint32_t)allocate_free_block();
        struct directory_item zeroes[BLOCK_SIZE / sizeof(struct directory_item)];
        memset(zeroes, 0, sizeof(zeroes));
        for (int i = 0; i < (int)(BLOCK_SIZE / sizeof(struct directory_item)); i++) {
            zeroes[i].inode_id = FS_INVALID_INODE;
        }
        write_block((int)inode.direct_blocks[0], zeroes);
    }

    write_inode(inode_id, &inode);
    add_directory_item(parent_inode, name, inode_id);
    return inode_id;
}

/**
 * Удаляет файл или директорию (если пустая) и удаляет упоминание в родительском каталоге.
 * @param path полный путь к файлу или каталогу
 */
int delete_file(const char* path) {
    const int inode_id = find_inode_by_path(path);
    if (inode_id < 0) {
        printf("ERROR: Path '%s' not found\n", path);
        return 1;
    }

    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (inode.is_directory && !is_directory_empty(inode_id)) {
        printf("Cannot delete non-empty directory (inode %d)\n", inode_id);
        return 2;
    }

    // Разделяем путь на родителя и имя
    char parent_path[MAX_PATH_LEN];
    char name[MAX_PATH_LEN];
    if (!split_path(path, parent_path, name)) {
        printf("ERROR: Invalid path '%s'\n", path);
        return 1;
    }

    int parent_inode = find_inode_by_path(parent_path);
    if (parent_inode < 0) {
        printf("ERROR: Parent path '%s' not found\n", parent_path);
        return 1;
    }

    // Удаляем элемент из родительской директории
    if (!remove_directory_item(parent_inode, name)) {
        printf("WARNING: Could not remove '%s' from parent directory\n", name);
        return 1;
    }

    // Освобождаем блоки
    if (inode.is_directory && inode.direct_blocks[0] != FS_INVALID_BLOCK)
        free_block((int)inode.direct_blocks[0]);

    if (!inode.is_directory) {
        for (int i = 0; i < 5; i++) {
            if (inode.direct_blocks[i] != FS_INVALID_BLOCK)
                free_block((int)inode.direct_blocks[i]);
        }
        if (inode.indirect_block != FS_INVALID_BLOCK) {
            uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
            read_block((int)inode.indirect_block, indirect_blocks);
            int count = BLOCK_SIZE / sizeof(uint32_t);
            for (int i = 0; i < count; i++) {
                if (indirect_blocks[i] != FS_INVALID_BLOCK)
                    free_block((int)indirect_blocks[i]);
            }
            free_block((int)inode.indirect_block);
        }
    }

    // Освобождаем inode
    free_inode(inode_id);
    return 0;
}

/**
 * Reads all data blocks of the given inode into a provided buffer.
 *
 * @param inode_id  ID of the inode to read from
 * @param buffer    Pointer to a buffer large enough to hold all data
 * @return          Total number of bytes read into the buffer
 *
 * The function automatically handles:
 *  - Direct data blocks (up to 5)
 *  - Single indirect block (array of block IDs)
 */
int read_inode_data(int inode_id, void* buffer) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (inode.is_directory) {
        printf("ERROR: inode %d is a directory, not a file\n", inode_id);
        return -1;
    }

    int total_read = 0;
    char block_data[BLOCK_SIZE];

    // =========================
    // 1️⃣  Read direct blocks
    // =========================
    for (int i = 0; i < 5; i++) {
        if (inode.direct_blocks[i] == FS_INVALID_BLOCK)
            continue;

        read_block((int)inode.direct_blocks[i], block_data);
        memcpy((char*)buffer + total_read, block_data, BLOCK_SIZE);
        total_read += BLOCK_SIZE;
    }

    // =========================
    // 2️⃣  Read single indirect block (if exists)
    // =========================
    if (inode.indirect_block != FS_INVALID_BLOCK) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
        read_block((int)inode.indirect_block, indirect_blocks);

        int count = BLOCK_SIZE / sizeof(uint32_t);
        for (int i = 0; i < count; i++) {
            if (indirect_blocks[i] == FS_INVALID_BLOCK)
                continue;

            read_block((int)indirect_blocks[i], block_data);
            memcpy((char*)buffer + total_read, block_data, BLOCK_SIZE);
            total_read += BLOCK_SIZE;
        }
    }

    // Ensure not to read beyond file_size
    if (total_read > inode.file_size)
        total_read = inode.file_size;

    return total_read;
}


/**
 * Writes data from a buffer into all data blocks of the given inode.
 *
 * @param inode_id  ID of the inode to write into
 * @param buffer    Pointer to data to be written
 * @param size      Number of bytes to write
 * @return          Number of bytes actually written
 *
 * This function automatically:
 *  - Allocates data blocks if needed (direct + indirect)
 *  - Handles single-level indirect addressing
 *  - Updates inode.file_size
 */
int write_inode_data(int inode_id, const void* buffer, int size) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (inode.is_directory) {
        printf("ERROR: inode %d is a directory, not a file\n", inode_id);
        return -1;
    }

    int bytes_written = 0;
    const char* data_ptr = (const char*)buffer;
    char block_data[BLOCK_SIZE];

    // =========================
    // 1️⃣ Write direct blocks
    // =========================
    for (int i = 0; i < 5 && bytes_written < size; i++) {
        if (inode.direct_blocks[i] == FS_INVALID_BLOCK)
            inode.direct_blocks[i] = (uint32_t)allocate_free_block();

        memset(block_data, 0, BLOCK_SIZE);
        int chunk = (size - bytes_written > BLOCK_SIZE) ? BLOCK_SIZE : (size - bytes_written);
        memcpy(block_data, data_ptr + bytes_written, chunk);
        write_block((int)inode.direct_blocks[i], block_data);
        bytes_written += chunk;
    }

    // =========================
    // 2️⃣ Write to indirect blocks (if needed)
    // =========================
    if (bytes_written < size) {
        uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];

        if (inode.indirect_block == FS_INVALID_BLOCK) {
            inode.indirect_block = (uint32_t)allocate_free_block();
            for (int k = 0; k < (int)(BLOCK_SIZE / sizeof(uint32_t)); k++) indirect_blocks[k] = FS_INVALID_BLOCK;
            write_block((int)inode.indirect_block, indirect_blocks);
        } else {
            read_block((int)inode.indirect_block, indirect_blocks);
        }

        int indirect_count = BLOCK_SIZE / sizeof(uint32_t);
        for (int j = 0; j < indirect_count && bytes_written < size; j++) {
            if (indirect_blocks[j] == FS_INVALID_BLOCK)
                indirect_blocks[j] = (uint32_t)allocate_free_block();

            memset(block_data, 0, BLOCK_SIZE);
            int chunk = (size - bytes_written > BLOCK_SIZE) ? BLOCK_SIZE : (size - bytes_written);
            memcpy(block_data, data_ptr + bytes_written, chunk);
            write_block((int)indirect_blocks[j], block_data);
            bytes_written += chunk;
        }

        write_block((int)inode.indirect_block, indirect_blocks);
    }

    // =========================
    // 3️⃣ Update inode info
    // =========================
    inode.file_size = bytes_written;
    write_inode(inode_id, &inode);

    return bytes_written;
}
