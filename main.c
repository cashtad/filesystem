#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "err.h"
#include "vfs_layers/shell/shell_layer.h"

static char *filesystem_name;

int main(const int argc, char *argv[]) {
    // Expect a single argument: path to VFS container file.
    if (argc < 2) {
        error_exit(ERROR_WRONG_ARGS_TEXT, ERROR_ARGS);
    }

    filesystem_name = argv[1];

    // Ensure unmount/flush on normal process termination.
    atexit(fs_unmount);

    run_shell(filesystem_name);
    return 0;
}
