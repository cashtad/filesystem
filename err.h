#ifndef FILE_SYSTEM_ERR_H
#define FILE_SYSTEM_ERR_H

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Error message for invalid arguments in the program.
 *
 * This message is displayed if the input arguments are invalid.
 */
#define ERROR_WRONG_ARGS_TEXT "invalid program arguments. Correct usage: filesystem <data>."

#define ERROR_WRONG_COMMAND_TEXT "command not found."


/**
 * @brief Error code for invalid arguments.
 *
 * This code represents an error related to invalid arguments passed to the program.
 */
#define ERROR_ARGS 1


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
void error_exit(const char *message, int exit_code);


#endif //FILE_SYSTEM_ERR_H