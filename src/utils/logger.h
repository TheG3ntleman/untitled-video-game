#ifndef LOGGER_H
#define LOGGER_H

/*
    The goal of the logger is to allow for an easy mechanism to put up messages and route them to appropriate streams.
*/

#include <stdbool.h>

typedef enum LogType {
    LOGGER_INFO=0,
    LOGGER_DEBUG,
    LOGGER_WARNING,
    LOGGER_ERROR,
} LogType;

typedef struct LoggerConfig {
    bool maintain_log_file;
    char log_file_path[512];
} LoggerConfig;

// typedef struct LoggerState LoggerState;

void logger_initialize(LoggerConfig config);

void logger_log(
    LogType type,
    const char *file,
    int line,
    const char *fmt,
    ...
);
void logger_destroy();

#define LOG_INFO(...) logger_log(LOGGER_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) logger_log(LOGGER_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) logger_log(LOGGER_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOGGER_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif