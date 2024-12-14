#include <stdlib.h>
#include <string.h>

#include "writestr.h"

gplayback_writestr_state writestr_create(size_t capacity) {
  gplayback_writestr_state state = {
      .out = malloc(capacity * sizeof(char)),
      .offset = 0,
      .capacity = capacity,
  };
  memset(state.out, 0, capacity);
  return state;
}

void writestr_free(gplayback_writestr_state state) { free(state.out); }
