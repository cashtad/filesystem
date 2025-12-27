#ifndef SHELL_LAYER_H
#define SHELL_LAYER_H

#include "../logic/logic_layer.h"

/**
 * @file shell_layer.h
 * @brief Command interface (Layer 4) for the virtual file system.
 *
 * Provides user-level commands similar to Linux shell utilities.
 * Each function corresponds to a filesystem command (cp, mv, ls, etc.)
 * and internally calls the logic layer functions.
 */

/**
 * @brief Starts an interactive VFS shell session.
 *
 * @param filesystem_name Path to the VFS container file to mount.
 */
void run_shell(const char *filesystem_name);

/**
 * @brief Executes a single command line entered by the user.
 *
 * The function parses the input and dispatches to corresponding fs_* handlers.
 *
 * @param input Command line string.
 */
void execute_command(const char *input);

/**
 * @brief Copies a file inside the VFS: cp s1 s2
 *
 * @param src Source path in VFS.
 * @param dest Destination path in VFS.
 * @return 0 on success, otherwise non-zero (shell prints mapped message).
 */
int fs_copy(char *src, char *dest);

/**
 * @brief Moves/renames an entry inside the VFS: mv s1 s2
 *
 * @param src Source path in VFS.
 * @param dest Destination path in VFS.
 * @return 0 on success, otherwise non-zero.
 */
int fs_move(char *src, char *dest);

/**
 * @brief Removes a file inside the VFS: rm s1
 *
 * @param path Path to file in VFS.
 * @return 0 on success, otherwise non-zero.
 */
int fs_remove(char *path);

/**
 * @brief Creates a directory inside the VFS: mkdir a1
 *
 * @param path Directory path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_mkdir(char *path);

/**
 * @brief Removes an empty directory inside the VFS: rmdir a1
 *
 * @param path Directory path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_rmdir(char *path);

/**
 * @brief Lists directory content: ls [a1]
 *
 * @param path Path or "." for current directory.
 * @return 0 on success, otherwise non-zero.
 */
int fs_ls(char *path);

/**
 * @brief Prints file content: cat s1
 *
 * @param path File path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_cat(char *path);

/**
 * @brief Changes current working directory: cd a1
 *
 * @param path Directory path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_cd(char *path);

/**
 * @brief Writes the current working directory into buffer.
 *
 * @param buffer Output pointer (implementation-specific).
 */
void fs_pwd(void *buffer);

/**
 * @brief Prints inode/block information for a path: info s1
 *
 * @param path File or directory path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_info(char *path);

/**
 * @brief Imports a host file into VFS: incp host_src vfs_dest
 *
 * @param src Host filesystem path.
 * @param dest Destination path in VFS.
 * @return 0 on success, otherwise non-zero.
 */
int fs_import(const char *src, const char *dest);

/**
 * @brief Exports a VFS file to host: outcp vfs_src host_dest
 *
 * @param src Source path in VFS.
 * @param dest Destination path on host.
 * @return 0 on success, otherwise non-zero.
 */
int fs_export(const char *src, const char *dest);

/**
 * @brief Loads commands from a host file and executes them sequentially: load s1
 *
 * Format: one command per line.
 *
 * @param filename Host filesystem path to a script file.
 * @return 0 on success, 1 if file cannot be opened.
 */
int fs_load_script(const char *filename);

/**
 * @brief Formats the VFS container and mounts it again.
 *
 * @param size Size in MB.
 * @return 0 on success, otherwise non-zero.
 */
int fs_format_cmd(int size);

/**
 * @brief Prints filesystem statistics.
 */
void fs_stat(void);

/**
 * @brief Expands relative path to absolute path based on current directory.
 *
 * @param path Input path (may be relative).
 * @return Newly allocated absolute path string (caller-owned) or original pointer.
 */
char* complete_path(char* path);

/**
 * @brief Creates a file s3 as concatenation of s1 and s2: xcp s1 s2 s3
 *
 * @param s1 First source file path.
 * @param s2 Second source file path.
 * @param s3 Destination file path.
 * @return 0 on success, otherwise non-zero.
 */
int fs_xcp(char* s1, char* s2, char* s3);

/**
 * @brief Appends content of s2 to the end of s1 inside VFS: add s1 s2
 *
 * @param s1 Destination file path (modified).
 * @param s2 Source file path (read-only).
 * @return 0 on success, otherwise non-zero.
 */
int fs_add(char* s1, char* s2);

#endif // SHELL_LAYER_H
