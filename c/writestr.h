#ifndef WRITESTR_H
#define WRITESTR_H

#include <stddef.h>

#define writestr(out, offset, capacity, ...)                                   \
  {                                                                            \
    int write_len = snprintf(out + offset, capacity - offset, __VA_ARGS__);    \
    bool should_realloc = false;                                               \
    while (write_len >= capacity - offset) {                                   \
      should_realloc = true;                                                   \
      capacity = capacity * 2;                                                 \
    }                                                                          \
    if (should_realloc) {                                                      \
      out = realloc(out, capacity * sizeof(char));                             \
      offset += snprintf(out + offset, capacity - offset, __VA_ARGS__);        \
      memset(out + offset, 0, capacity - offset);                              \
    } else {                                                                   \
      offset += write_len;                                                     \
    }                                                                          \
  }
#endif // !WRITESTR_H
