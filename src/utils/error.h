#ifndef ERROR_H
#define ERROR_H

typedef enum ErrorCode {
    GENERAL_ERROR=0,
} ErrorCode;

void throw_error(
    ErrorCode code,
    const char *file,
    int line,
    const char *fmt,
    ...
);

#define ERROR(...) throw_error(GENERAL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif