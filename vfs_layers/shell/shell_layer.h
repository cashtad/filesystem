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

// Initializes the shell (e.g., sets current directory to "/")
void shell_init();

// Executes a single user command (e.g. "cp file1 file2")
void execute_command(const char *input);

// Basic commands
void cmd_cp(const char *src, const char *dest);
void cmd_mv(const char *src, const char *dest);
void cmd_rm(const char *path);
void cmd_mkdir(const char *path);
void cmd_rmdir(const char *path);
void cmd_ls(const char *path);
void cmd_cat(const char *path);
void cmd_cd(const char *path);
void cmd_pwd();
void cmd_info(const char *path);
void cmd_incp(const char *src, const char *dest);
void cmd_outcp(const char *src, const char *dest);
void cmd_load(const char *filename);
void cmd_format(const char *size_str);
void cmd_statfs();

// Exits the shell
void cmd_exit();

#endif // SHELL_LAYER_H
