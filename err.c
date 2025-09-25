#include "err.h"

/**
 * @brief Prints an error message and exits the program with the specified exit code.
 *
 * This function is used to display an error message to the standard error stream and then terminate the program with the provided exit code.
 * It is typically used when an irrecoverable error occurs, and the program needs to exit immediately.
 *
 * @param message The error message to be printed.
 * @param exit_code The exit code that the program will return upon termination.
 *
 * @note The program will terminate after this function is called, so no code after the call will be executed.
 */
void error_exit(const char *message, const int exit_code) {
    fprintf(stderr, "Error: %s.\n", message);
    exit(exit_code);
}
