#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

typedef struct LoggerState {
    LoggerConfig config;
    FILE *log_file;
    bool initialized;
} LoggerState;

static LoggerState global_logger_state = {0};

void logger_initialize(LoggerConfig config) {
    global_logger_state.config = config;

    if (config.maintain_log_file) {
        // We are going to maintain a log file.

        global_logger_state.log_file = fopen(global_logger_state.config.log_file_path, "a");
        if (!global_logger_state.log_file) {
            perror("fopen");
        }
    }

    global_logger_state.initialized = true;


    fprintf(stdout, "%s", "\n=============================================================================================================\n");
    fflush(stdout);

    if (global_logger_state.config.maintain_log_file && global_logger_state.log_file) {
        fprintf(global_logger_state.log_file, "%s", "\n=============================================================================================================\n");
        fflush(global_logger_state.log_file);
    }
}

void logger_destroy() {
    if (global_logger_state.config.maintain_log_file) {
        fclose(global_logger_state.log_file);
        global_logger_state.log_file = NULL;
    }

    global_logger_state.initialized = false;
}

static const char *log_type_to_string(LogType type) {
    switch (type) {
        case LOGGER_DEBUG: return "DEBUG";
        case LOGGER_WARNING: return "WARNING";
        case LOGGER_ERROR: return "ERROR";
        case LOGGER_INFO: return "INFO";
        default: return "UNKNOWN";//perror("Unknown Log Type");
    }
}

void logger_log(
    LogType type,
    const char *file,
    int line,
    const char *fmt, 
    ...
) {

    if (!global_logger_state.initialized) perror("Logger not intialized!");

    char message[1024];
    char timestamp[64];

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local);
    } else {
        snprintf(timestamp, sizeof(timestamp), "unknown-time");
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    FILE *stream = (type == LOGGER_ERROR || type == LOGGER_WARNING) ? stderr : stdout;

    fprintf(stream, "| %-12s | %-20s | ...%20s:%d | %s\n",
            log_type_to_string(type), timestamp, file, line,message);
    fflush(stream);

    if (global_logger_state.config.maintain_log_file && global_logger_state.log_file) {
        fprintf(global_logger_state.log_file, "| %-12s | %-20s | ...%20s:%d | %s\n",
                log_type_to_string(type), timestamp, file, line, message);
        fflush(global_logger_state.log_file);
    }

}
