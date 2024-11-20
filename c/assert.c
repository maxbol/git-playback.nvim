#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef LUA_EXCEPTIONS
#include <lauxlib.h>
#include <lua.h>

lua_State *L = NULL;

void set_err_lua_state(lua_State *state) { L = state; }
void clear_err_lua_state() { L = NULL; }

void error_f(const char *message) {
  if (L == NULL) {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
  luaL_error(L, "Assertion failed: %s\n", message);
}
#else
void set_err_lua_state(void *state) {}
void clear_err_lua_state() {}
void error_f(const char *message) {
  fprintf(stderr, "Assertion failed: %s\n", message);
  exit(1);
}
#endif /* if LUA_EXCEPTIONS */
