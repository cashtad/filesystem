#include "err.h"

// Print error message to stderr and terminate the process with the given exit code.
void error_exit(const char *message, const int exit_code) {
    fprintf(stderr, "Error: %s.\n", message);
    exit(exit_code);
}
