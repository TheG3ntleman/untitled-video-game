#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "error.h"
#include "logger.h"

static const char *error_code_to_string(ErrorCode code) {
    switch (code) {
        case GENERAL_ERROR: return "GENERAL ERROR";
        default: return "UNKNOWN ERROR";
    }
}

void throw_error(
    ErrorCode code,
    const char *file,
    int line,
    const char *fmt,
    ...
) {


    char message[2048];
    char full_error_message[2048];

    // We will write stuff to message and also apply the formatter.
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // Now we format the full error message

    snprintf(
        full_error_message,
        sizeof(full_error_message),
        "%s | %s", 
        error_code_to_string(code),
        message
    );

    logger_log(
        LOGGER_ERROR,
        file,
        line,
        "%s",
        full_error_message
    );

    // Gracefully exiting
    exit(1);
}