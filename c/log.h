#ifndef LOG_H
#define LOG_H

#ifndef OUT_BUFFER_SIZE
#define OUT_BUFFER_SIZE 16384
#endif // !OUT_BUFFER_SIZE

#ifdef ENABLE_DEBUG_LOGGING
#include <lua.h>

#include "escapestr.h"
#include "luastate.h"
#include "writestr.h"

#define _dbg_log(escape, ...)                                                  \
  do {                                                                         \
    log_lua_state L = get_lua_state();                                         \
    char out[OUT_BUFFER_SIZE] = {0};                                           \
    if (escape) {                                                              \
      escape_fmt(out, sizeof(out), __VA_ARGS__);                               \
    } else {                                                                   \
      snprintf(out, sizeof(out), __VA_ARGS__);                                 \
    }                                                                          \
                                                                               \
    if (L == NULL) {                                                           \
      fprintf(stderr, "DEBUG: %s:%d: ", __FILE__, __LINE__);                   \
      fputs(out, stderr);                                                      \
      fputs("\n", stderr);                                                     \
    } else {                                                                   \
      lua_getglobal(L, "print");                                               \
      lua_pushfstring(L, "DEBUG: %s:%d", __FILE__, __LINE__);                  \
      lua_pushstring(L, out);                                                  \
      lua_call(L, 2, 0);                                                       \
    }                                                                          \
  } while (0)

#define dbg_log(...) _dbg_log(true, __VA_ARGS__)
#define dbg_log_raw(...) _dbg_log(false, __VA_ARGS__)

#else
#include <stdio.h>
#define dbg_log(...) snprintf(NULL, 0, __VA_ARGS__);
#define dbg_log_raw(...) snprintf(NULL, 0, __VA_ARGS__);
#endif // !ENABLE_DEBUG_LOGGING

#endif // !LOG_H
