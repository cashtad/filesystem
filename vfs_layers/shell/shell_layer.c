#include "shell_layer.h"
#include "../logic/logic_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================================================================
// 🔹 Helper functions
// ================================================================

/**
 * @brief Executes a single command line by parsing its arguments
 */
void execute_command(const char *input) {
    char cmd[64], arg1[256], arg2[256];
    int args = sscanf(input, "%63s %255s %255s", cmd, arg1, arg2);

    if (args < 1) return;

    // ================================================================
    // 1) cp s1 s2
    // ================================================================
    if (strcmp(cmd, "cp") == 0) {
        if (args < 3) { printf("Usage: cp s1 s2\n"); return; }
        int res = fs_copy(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 2) mv s1 s2
    // ================================================================
    else if (strcmp(cmd, "mv") == 0) {
        if (args < 3) { printf("Usage: mv s1 s2\n"); return; }
        int res = fs_move(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 3) rm s1
    // ================================================================
    else if (strcmp(cmd, "rm") == 0) {
        if (args < 2) { printf("Usage: rm s1\n"); return; }
        int res = fs_remove(arg1);
        if (res == 0) printf("OK\n");
        else printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 4) mkdir a1
    // ================================================================
    else if (strcmp(cmd, "mkdir") == 0) {
        if (args < 2) { printf("Usage: mkdir a1\n"); return; }
        int res = fs_mkdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("PATH NOT FOUND\n");
        else if (res == 2) printf("EXIST\n");
    }

    // ================================================================
    // 5) rmdir a1
    // ================================================================
    else if (strcmp(cmd, "rmdir") == 0) {
        if (args < 2) { printf("Usage: rmdir a1\n"); return; }
        int res = fs_rmdir(arg1);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("NOT EMPTY\n");
    }

    // ================================================================
    // 6) ls [a1]
    // ================================================================
    else if (strcmp(cmd, "ls") == 0) {
        if (args == 1) fs_ls(".");
        else if (args >= 2) fs_ls(arg1);
    }

    // ================================================================
    // 7) cat s1
    // ================================================================
    else if (strcmp(cmd, "cat") == 0) {
        if (args < 2) { printf("Usage: cat s1\n"); return; }
        int res = fs_cat(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 8) cd a1
    // ================================================================
    else if (strcmp(cmd, "cd") == 0) {
        if (args < 2) { printf("Usage: cd a1\n"); return; }
        int res = fs_cd(arg1);
        if (res == 0) printf("OK\n");
        else printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 9) pwd
    // ================================================================
    else if (strcmp(cmd, "pwd") == 0) {
        char path[512];
        fs_pwd(path);
        printf("%s\n", path);
    }

    // ================================================================
    // 10) info s1
    // ================================================================
    else if (strcmp(cmd, "info") == 0) {
        if (args < 2) { printf("Usage: info s1\n"); return; }
        int res = fs_info(arg1);
        if (res == 1) printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 11) incp s1 s2
    // ================================================================
    else if (strcmp(cmd, "incp") == 0) {
        if (args < 3) { printf("Usage: incp s1 s2\n"); return; }
        int res = fs_import(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 12) outcp s1 s2
    // ================================================================
    else if (strcmp(cmd, "outcp") == 0) {
        if (args < 3) { printf("Usage: outcp s1 s2\n"); return; }
        int res = fs_export(arg1, arg2);
        if (res == 0) printf("OK\n");
        else if (res == 1) printf("FILE NOT FOUND\n");
        else if (res == 2) printf("PATH NOT FOUND\n");
    }

    // ================================================================
    // 13) load s1
    // ================================================================
    else if (strcmp(cmd, "load") == 0) {
        if (args < 2) { printf("Usage: load s1\n"); return; }
        int res = fs_load_script(arg1);
        if (res == 0) printf("OK\n");
        else printf("FILE NOT FOUND\n");
    }

    // ================================================================
    // 14) format <sizeMB>
    // ================================================================
    else if (strcmp(cmd, "format") == 0) {
        if (args < 2) { printf("Usage: format <sizeMB>\n"); return; }
        int size = atoi(arg1);
        int res = fs_format(size);
        if (res == 0) printf("OK\n");
        else printf("CANNOT CREATE FILE\n");
    }

    // ================================================================
    // 15) exit
    // ================================================================
    else if (strcmp(cmd, "exit") == 0) {
        fs_unmount();
        printf("Exiting...\n");
        exit(0);
    }

    // ================================================================
    // 16) statfs
    // ================================================================
    else if (strcmp(cmd, "statfs") == 0) {
        fs_stat();
    }

    // ================================================================
    // Unknown command
    // ================================================================
    else {
        printf("Unknown command\n");
    }
}

// ================================================================
// 🔹 Shell main loop
// ================================================================

void run_shell() {
    printf("=== Virtual File System Shell ===\n");
    printf("Type 'help' for list of supported commands.\n");

    char input[1024];
    while (1) {
        printf("vfs> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        execute_command(input);
    }
}
