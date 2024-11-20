#ifndef ASSERT_H
#define ASSERT_H
#include <stdio.h>

#ifdef LUA_EXCEPTIONS
#include <lua.h>
void set_err_lua_state(lua_State *state);
#else
void set_err_lua_state(void *state);
#endif
void clear_err_lua_state();
void error_f(const char *message);

#define error(...)                                                             \
  {                                                                            \
    char out[4096];                                                            \
    snprintf(out, 4096, __VA_ARGS__);                                          \
    error_f(out);                                                              \
  }

#define assert(condition, ...)                                                 \
  if (!(condition)) {                                                          \
    error(__VA_ARGS__);                                                        \
  }

#endif /* ASSERT_H */
