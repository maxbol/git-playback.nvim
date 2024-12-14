#ifndef ESCAPESTR_H
#define ESCAPESTR_H

#include "writestr.h"

#define escape_fmt(v_escaped, v_escaped_len, ...)                              \
  ({                                                                           \
    gplayback_writestr_state ws = writestr_create(512);                        \
    writestr(ws, __VA_ARGS__);                                                 \
    int len = escape_string(ws.out, v_escaped, v_escaped_len);                 \
    writestr_free(ws);                                                         \
    len;                                                                       \
  })

size_t escape_string(const char *str, char escaped[], size_t escaped_len);

#endif // !ESCAPESTR_H
