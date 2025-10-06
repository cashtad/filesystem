//
// Created by Leonid on 25.09.2025.
//

#ifndef FILE_SYSTEM_OUTPUT_H
#define FILE_SYSTEM_OUTPUT_H


static void processing_command(const char *command) {
    if (command) printf("Processing the command: :%s\n", command);
    else printf("Processing the command... \n");
}

static void print_ok(const char *msg) {
    if (msg) printf("OK: %s\n", msg);
    else printf("OK\n");
}

static void print_err(const char *msg) {
    if (msg) fprintf(stderr, "ERR: %s\n", msg);
    else fprintf(stderr, "ERR\n");
}

#endif //FILE_SYSTEM_OUTPUT_H
