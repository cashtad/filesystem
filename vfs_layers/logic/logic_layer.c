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

    if (inode.direct_blocks[0] == 0)
        return true; // у директории нет даже блока — значит она пустая

    const int items = BLOCK_SIZE / sizeof(struct directory_item);


    struct directory_item buffer[items]; // выделяем массив для позиций в папке

    read_block((int) inode.direct_blocks[0], buffer);


    for (int j = 0; j < items; j++) {
        if (buffer[j].inode_id != 0)
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

    if (inode.direct_blocks[0] == 0)
        return -1; // директория пуста

    const int items = BLOCK_SIZE / sizeof(struct directory_item);


    struct directory_item buffer[items];
    read_block((int) inode.direct_blocks[0], buffer);

    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id != 0 && strcmp(buffer[i].name, name) == 0)
            return (int) buffer[i].inode_id;
    }

    return -1;
}

/**
 * Добавляет элемент (файл/каталог) в директорию.
 * Возвращает true при успехе, false если каталог переполнен.
 */
bool add_directory_item(const int parent_inode, const char* name, const int child_inode) {
    struct pseudo_inode inode;

    // читаем ноду папки
    read_inode(parent_inode, &inode);

    if (!inode.is_directory) { // проверка, если это вообще папка
        printf("ERROR: inode %d is not a directory\n", parent_inode);
        return false;
    }

    // Если у папки ещё нет блока — создаём (если она пустая)
    if (inode.direct_blocks[0] == 0) {
        inode.direct_blocks[0] = allocate_free_block(); // выделяем блок
        write_inode(parent_inode, &inode);

        // Заполняем блок нулями
        const struct directory_item zeroes[BLOCK_SIZE / sizeof(struct directory_item)] = {0};
        write_block((int) inode.direct_blocks[0], zeroes);
    }

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)]; // баффер файлов в этой папке
    read_block((int) inode.direct_blocks[0], buffer); // читаем в баффер
    const int items = BLOCK_SIZE / sizeof(struct directory_item); // количество возможных предметов в папке

    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id == 0) { // нашлась пустая позиция (есть свободное место)
            strcpy(buffer[i].name, name);
            buffer[i].inode_id = child_inode;
            write_block((int) inode.direct_blocks[0], buffer); // закинули инфу, записали на диск, вернули true
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

    if (inode.direct_blocks[0] == 0) // если блок не выделен, значит, что каталог пуст
        return false;

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    read_block((int) inode.direct_blocks[0], buffer);
    const int items = BLOCK_SIZE / sizeof(struct directory_item);

    for (int i = 0; i < items; i++) {
        if (strcmp(buffer[i].name, name) == 0) { // если нашли ноду с таким именем, то стираем инфу
            buffer[i].inode_id = 0;
            buffer[i].name[0] = '\0';
            write_block((int) inode.direct_blocks[0], buffer); // перезаписываем блок (без того элемента)
            return true;
        }
    }

    return false;
}

/**
 * Выводит список содержимого директории.
 */
void list_directory(const int inode_id) {
    struct pseudo_inode inode;
    read_inode(inode_id, &inode);

    if (!inode.is_directory) {
        printf("ERROR: inode %d is not a directory\n", inode_id);
        return;
    }

    if (inode.direct_blocks[0] == 0) {
        printf("(empty directory)\n");
        return;
    }

    struct directory_item buffer[BLOCK_SIZE / sizeof(struct directory_item)];
    read_block((int) inode.direct_blocks[0], buffer);
    const int items = BLOCK_SIZE / sizeof(struct directory_item);

    printf("Contents of directory (inode %d):\n", inode_id);
    for (int i = 0; i < items; i++) {
        if (buffer[i].inode_id != 0)
            printf("  %s (inode %d)\n", buffer[i].name, buffer[i].inode_id);
    }
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

    if (isDirectory) {
        // Для директории сразу выделяем один блок под содержимое
        inode.direct_blocks[0] = allocate_free_block();
        struct directory_item zeroes[BLOCK_SIZE / sizeof(struct directory_item)] = {0};
        write_block((int) inode.direct_blocks[0], zeroes);
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
    if (inode.is_directory && inode.direct_blocks[0] != 0)
        free_block((int) inode.direct_blocks[0]);

    if (!inode.is_directory) {
        for (int i = 0; i < 5; i++) {
            if (inode.direct_blocks[i] != 0)
                free_block((int) inode.direct_blocks[i]);
        }
        if (inode.indirect_block != 0) {
            uint32_t indirect_blocks[BLOCK_SIZE / sizeof(uint32_t)];
            read_block((int) inode.indirect_block, indirect_blocks);
            int count = BLOCK_SIZE / sizeof(uint32_t);
            for (int i = 0; i < count; i++) {
                if (indirect_blocks[i] != 0)
                    free_block((int) indirect_blocks[i]);
            }
            free_block((int) inode.indirect_block);
        }
    }

    // Освобождаем inode
    free_inode(inode_id);
    return 0;
}
