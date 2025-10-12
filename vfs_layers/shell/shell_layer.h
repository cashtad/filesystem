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

void run_shell();

// Executes a single user command (e.g. "cp file1 file2")
void execute_command(const char *input);

// Basic commands
int fs_copy(char *src, char *dest);
int fs_move(char *src, char *dest);
int fs_remove(char *path);
int fs_mkdir(char *path);
int fs_rmdir(char *path);
int fs_ls(char *path);
int fs_cat(char *path);
int fs_cd(char *path);
void fs_pwd(void *buffer);
int fs_info(char *path);
int fs_import(const char *src, const char *dest);
int fs_export(const char *src, const char *dest);
int fs_load_script(const char *filename);
int fs_format_cmd(int size);
void fs_stat();
char* complete_path(char* path);

#endif // SHELL_LAYER_H
