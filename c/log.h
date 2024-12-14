#ifndef LOG_H
#define LOG_H

#ifndef ESCAPE_BUFFER_SIZE
#define ESCAPE_BUFFER_SIZE 4096
#endif // !ESCAPE_BUFFER_SIZE

#ifdef ENABLE_DEBUG_LOGGING
#include "escapestr.h"
#include "writestr.h"

#define dbg_log(...)                                                           \
  do {                                                                         \
    fprintf(stderr, "DEBUG: %s:%d: ", __FILE__, __LINE__);                     \
    char escaped[ESCAPE_BUFFER_SIZE] = {0};                                    \
    escape_fmt(escaped, sizeof(escaped), __VA_ARGS__);                         \
    fputs(escaped, stderr);                                                    \
    fputs("\n", stderr);                                                       \
  } while (0)
#else
#include <stdio.h>
#define dbg_log(...) snprintf(NULL, 0, __VA_ARGS__);
#endif // !ENABLE_DEBUG_LOGGING

#endif // !LOG_H
