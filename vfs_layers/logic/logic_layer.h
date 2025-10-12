#ifndef FILE_SYSTEM_LOGIC_LAYER_H
#define FILE_SYSTEM_LOGIC_LAYER_H

#include "../meta/meta_layer.h"   // содержит pseudo_inode и directory_item
#include <string.h>
#include <stdlib.h>

#define MAX_FILENAME_LEN 12
#define MAX_PATH_LEN 256

/**
 * Инициализация базовых структур логического уровня.
 * Создает root-директорию, если она отсутствует.
 */
void logic_init(void);

/**
 * Поиск inode по абсолютному или относительному пути.
 * Например: "/", "/dir1/dir2/file.txt".
 * Возвращает ID inode или -1, если не найден.
 */
int find_inode_by_path(const char* path);

/**
 * Проверяет, существует ли данный путь.
 * Возвращает true, если inode найден.
 */
bool path_exists(const char* path);

/**
 * Проверяет, является ли inode директорией.
 */
bool is_directory(int inode_id);

/**
 * Добавляет элемент (файл или директорию) в каталог.
 * @param parent_inode - inode родительского каталога.
 * @param name - имя нового элемента (<= 12 символов).
 * @param child_inode - inode добавляемого элемента.
 * Возвращает true, если успешно.
 */
bool add_directory_item(int parent_inode, const char* name, int child_inode);

/**
 * Удаляет элемент с заданным именем из каталога.
 * Возвращает true, если элемент был найден и удалён.
 */
bool remove_directory_item(int parent_inode, const char* name);

/**
 * Ищет элемент в каталоге по имени.
 * Возвращает inode_id или -1, если не найден.
 */
int find_item_in_directory(int parent_inode, const char* name);

/**
 * Выводит список всех элементов каталога.
 */
void list_directory(int inode_id);

/**
 * Создаёт файл или директорию.
 * @param parent_inode - inode родителя.
 * @param name - имя нового файла/директории.
 * @param isDirectory - true для директории, false для файла.
 * Возвращает inode_id созданного объекта, либо -1 при ошибке.
 */
int create_file(int parent_inode, const char* name, bool isDirectory);

/**
 * Удаляет файл или директорию (если она пустая).
 */
void delete_file(const char* path);

#endif // FILE_SYSTEM_LOGIC_LAYER_H
