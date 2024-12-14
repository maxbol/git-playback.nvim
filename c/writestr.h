#ifndef WRITESTR_H
#define WRITESTR_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "assert.h"

typedef struct {
  char *out;
  size_t offset;
  size_t capacity;
} gplayback_writestr_state;

#define writestr(state, ...)                                                   \
  {                                                                            \
    int write_len = snprintf((state).out + (state.offset),                     \
                             (state.capacity) - (state.offset), __VA_ARGS__);  \
    bool should_realloc = false;                                               \
    while ((state.offset) + write_len >= (state.capacity)) {                   \
      should_realloc = true;                                                   \
      (state.capacity) *= 2;                                                   \
    }                                                                          \
    if (should_realloc) {                                                      \
      (state).out = realloc((state).out, (state.capacity) * sizeof(char));     \
      assert(snprintf((state).out + (state.offset),                            \
                      (state.capacity) - (state.offset),                       \
                      __VA_ARGS__) == write_len,                               \
             "snprintf() wrote fewer bytes than intended after realloc");      \
      memset((state).out + (state.offset) + write_len, 0,                      \
             (state.capacity) - (state.offset) - write_len);                   \
    }                                                                          \
    (state.offset) += write_len;                                               \
  }

gplayback_writestr_state writestr_create(size_t capacity);
void writestr_free(gplayback_writestr_state state);

#endif // !WRITESTR_H
