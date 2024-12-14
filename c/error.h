#ifndef ERROR_H
#define ERROR_H
#include <stdio.h>

#ifdef LUA_EXCEPTIONS
#include <lua.h>
void set_err_lua_state(lua_State *state);
#else
void set_err_lua_state(void *state);
#endif
void clear_err_lua_state();
void error_f(const char *message);
void raw_error_f(const char *message);
void pure_error_f(const char *message);

#define error(...)                                                             \
  {                                                                            \
    char out[4096];                                                            \
    int offset = snprintf(out, 4096, "ERROR: %s:%d: ", __FILE__, __LINE__);    \
    snprintf(out + offset, 4096 - offset, __VA_ARGS__);                        \
    error_f(out);                                                              \
  }

#endif /* ERROR_H */
