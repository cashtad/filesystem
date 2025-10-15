#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "err.h"
// #include "commands.h"
// #include "output.h"
#include "vfs_layers/shell/shell_layer.h"

static char *filesystem_name;


int main(const int argc, char *argv[]) {
    if (argc < 2) {
        error_exit(ERROR_WRONG_ARGS_TEXT,ERROR_ARGS);
    }

    filesystem_name = argv[1];

    run_shell(filesystem_name);

}

