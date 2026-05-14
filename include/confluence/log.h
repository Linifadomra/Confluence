#ifndef CONFLUENCE_LOG_H
#define CONFLUENCE_LOG_H

#include <stdio.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 1
#endif

#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO  1
#define LOG_LVL_WARN  2
#define LOG_LVL_ERROR 3
#define LOG_LVL_OFF   4

#define LOG_AT(lvl, tag, ...) do { \
    if ((lvl) >= LOG_LEVEL) { \
        fprintf(stderr, "[%s] ", tag); \
        fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr); \
    } \
} while (0)

#define LOG_DEBUG(...) LOG_AT(LOG_LVL_DEBUG, "debug", __VA_ARGS__)
#define LOG_INFO(...)  LOG_AT(LOG_LVL_INFO,  "info",  __VA_ARGS__)
#define LOG_WARN(...)  LOG_AT(LOG_LVL_WARN,  "warn",  __VA_ARGS__)
#define LOG_ERROR(...) LOG_AT(LOG_LVL_ERROR, "error", __VA_ARGS__)

#endif
