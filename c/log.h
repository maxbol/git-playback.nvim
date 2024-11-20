#ifndef LOG_H
#define LOG_H

#ifdef ENABLE_DEBUG_LOGGING
#define dbg_log(...) fprintf(stderr, __VA_ARGS__);
#else
#include <stdio.h>
#define dbg_log(...) snprintf(NULL, 0, __VA_ARGS__);
#endif // !ENABLE_DEBUG_LOGGING

#endif // !LOG_H
