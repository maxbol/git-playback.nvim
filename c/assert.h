#ifndef ASSERT_H
#define ASSERT_H
#include <stdio.h>

#define LUA_EXCEPTIONS true

void set_err_lua_state(void *state);
void clear_err_lua_state();
void error_f(const char *message);

#define error(...)                                                             \
  {                                                                            \
    char out[1024];                                                            \
    snprintf(out, 1024, __VA_ARGS__);                                          \
    error_f(out);                                                              \
  }

#define assert(condition, ...)                                                 \
  if (!(condition)) {                                                          \
    error(__VA_ARGS__);                                                        \
  }

#endif /* ASSERT_H */
