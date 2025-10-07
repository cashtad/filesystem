#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "err.h"
#include "commands.h"
#include "output.h"

static char *filesystem_name;


bool prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

int define_command(const char *command) {
    if (prefix(copy, command))
        return copy_code;
    if (prefix(move, command)) {
        return move_code;
    }
    if (prefix(remove, command)) {
        return remove_code;
    }
    if (prefix(make_directory, command)) {
        return make_directory_code;
    }
    if (prefix(remove_directory, command)) {
        return remove_directory_code;
    }
    if (prefix(list_contents, command)) {
        return list_contents_code;
    }
    if (prefix(contains_at, command)) {
        return contains_at_code;
    }
    if (prefix(change_directory, command)) {
        return change_directory_code;
    }
    if (prefix(path, command)) {
        return path_code;
    }
    if (prefix(info, command)) {
        return info_code;
    }
    if (prefix(move_into_filesystem, command)) {
        return move_into_filesystem_code;
    }
    if (prefix(move_out_of_filesystem, command)) {
        return move_out_of_filesystem_code;
    }
    if (prefix(load_script, command)) {
        return load_script_code;
    }
    if (prefix(format, command)) {
        return format_code;
    }
    if (prefix(exit_manual, command)) {
        return exit_manual_code;
    }
    if (prefix(statistics, command)) {
        return statistics_code;
    }
    return wrong_command_code;
}
//
// int main(const int argc, char *argv[]) {
//     if (argc < 2) {
//         error_exit(ERROR_WRONG_ARGS_TEXT,ERROR_ARGS);
//     }
//
//     filesystem_name = argv[1];
//
//
//     printf("Currently working on filesystem: %s\n", filesystem_name);
//
//     while (true) {
//         char command[30];
//
//         scanf("%24s", command);
//
//         const int command_code = define_command(command);
//
//         switch (command_code) {
//             case 99:
//                 printf(ERROR_WRONG_COMMAND_TEXT "\n");
//                 break;
//
//             case copy_code:
//                 processing_command(copy);
//                 break;
//             case move_code:
//                 processing_command(move);
//                 break;
//
//             case remove_code:
//                 processing_command(remove);
//                 break;
//
//             case make_directory_code:
//                 processing_command(make_directory);
//                 break;
//
//             case remove_directory_code:
//                 processing_command(remove_directory);
//                 break;
//
//             case list_contents_code:
//                 processing_command(list_contents);
//                 break;
//
//             case contains_at_code:
//                 processing_command(contains_at);
//                 break;
//
//             case change_directory_code:
//                 processing_command(change_directory);
//                 break;
//
//             case path_code:
//                 processing_command(path);
//                 break;
//
//             case info_code:
//                 processing_command(info);
//                 break;
//
//             case move_into_filesystem_code:
//                 processing_command(move_into_filesystem);
//                 break;
//
//             case move_out_of_filesystem_code:
//                 processing_command(move_out_of_filesystem);
//                 break;
//
//             case load_script_code:
//                 processing_command(load_script);
//                 break;
//
//             case format_code:
//                 processing_command(format);
//                 break;
//
//             case exit_manual_code:
//                 processing_command(exit_manual);
//                 return (0);
//
//             default:
//                 printf(ERROR_WRONG_COMMAND_TEXT "\n");
//                 break;
//         }
//     }
// }

