#ifndef FILE_SYSTEM_LOGIC_LAYER_H
#define FILE_SYSTEM_LOGIC_LAYER_H

#include "../meta/meta_layer.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Maximum file or directory name length stored in directory entries.
 *
 * Must match struct directory_item::name capacity.
 */
#define MAX_FILENAME_LEN 12

/**
 * @brief Maximum supported path length (including terminating null).
 */
#define MAX_PATH_LEN 256

/**
 * @brief Maximum file size supported by the current inode addressing scheme.
 *
 * Computed as: 5 direct blocks + (BLOCK_SIZE / sizeof(uint32_t)) indirect blocks.
 */
#define MAX_FILE_SIZE ((5 + (BLOCK_SIZE / sizeof(uint32_t))) * BLOCK_SIZE)

/**
 * @brief Initializes logic layer state.
 *
 * Typically called after mounting and metadata initialization.
 * Ensures the filesystem is ready for path/directory operations.
 */
void logic_init(void);

/**
 * @brief Finds inode id by an absolute or relative path.
 *
 * @param path Input path (e.g. "/", "/dir/file.txt").
 * @return Inode id on success, -1 if not found.
 */
int find_inode_by_path(const char* path);

/**
 * @brief Checks whether a given path exists in the VFS.
 *
 * @param path Input path.
 * @return true if inode exists, false otherwise.
 */
bool path_exists(const char* path);

/**
 * @brief Checks whether the inode represents a directory.
 *
 * @param inode_id Inode id.
 * @return true if directory, false otherwise.
 */
bool is_directory(int inode_id);

/**
 * @brief Adds an entry (file or directory) into a parent directory.
 *
 * @param parent_inode Parent directory inode id.
 * @param name Entry name (<= MAX_FILENAME_LEN-1).
 * @param child_inode Child inode id to link.
 * @return true if added, false on failure (e.g. directory full).
 */
bool add_directory_item(int parent_inode, const char* name, int child_inode);

/**
 * @brief Removes an entry with the given name from a parent directory.
 *
 * @param parent_inode Parent directory inode id.
 * @param name Entry name.
 * @return true if removed, false if not found or invalid parent.
 */
bool remove_directory_item(int parent_inode, const char* name);

/**
 * @brief Finds a child entry by name inside a directory inode.
 *
 * @param parent_inode Directory inode id.
 * @param name Entry name.
 * @return Child inode id, or -1 if not found.
 */
int find_item_in_directory(int parent_inode, const char* name);

/**
 * @brief Prints directory content similar to 'ls'.
 *
 * @param inode_id Directory inode id.
 */
void list_directory(int inode_id);

/**
 * @brief Creates a file or directory and links it into the parent directory.
 *
 * @param parent_inode Parent directory inode id.
 * @param name New entry name.
 * @param isDirectory true to create directory, false to create regular file.
 * @return New inode id on success, -1 on failure.
 */
int create_file(int parent_inode, const char* name, bool isDirectory);

/**
 * @brief Deletes a file or an empty directory at the given path.
 *
 * @param path Full path to the entry to delete.
 * @return 0 on success, non-zero error code on failure.
 */
int delete_file(const char* path);

/**
 * @brief Splits a path into parent directory path and last component name.
 *
 * Example: "/a/b/c" -> parent="/a/b", name="c".
 *
 * @param path Input path.
 * @param parent Output buffer for parent path (MAX_PATH_LEN).
 * @param name Output buffer for name (MAX_FILENAME_LEN).
 * @return true on success, false on parse/validation error.
 */
bool split_path(const char* path, char* parent, char* name);

/**
 * @brief Checks whether a directory inode has no valid entries.
 *
 * @param inode_id Directory inode id.
 * @return true if empty, false otherwise.
 */
bool is_directory_empty(int inode_id);

/**
 * @brief Reads file content of an inode into the provided buffer.
 *
 * @param inode_id File inode id.
 * @param buffer Output buffer, must be large enough (typically MAX_FILE_SIZE).
 * @return Number of bytes read, or -1 on error.
 */
int read_inode_data(int inode_id, void* buffer);

/**
 * @brief Writes data into a file inode, allocating blocks if needed.
 *
 * This overwrites previous file content and updates inode.file_size.
 *
 * @param inode_id File inode id.
 * @param buffer Input data.
 * @param size Number of bytes to write.
 * @return Number of bytes written, or -1 on error.
 */
int write_inode_data(int inode_id, const void* buffer, int size);

#endif // FILE_SYSTEM_LOGIC_LAYER_H
