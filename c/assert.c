#include <lauxlib.h>
#include <lua.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define LUA_EXCEPTIONS true

lua_State *L = NULL;

void set_err_lua_state(lua_State *state) { L = state; }
void clear_err_lua_state() { L = NULL; }

#if LUA_EXCEPTIONS
void assert(bool condition, const char *message) {
  if (!condition) {
    if (L == NULL) {
      fprintf(stderr, "Lua state is NULL\n");
      exit(1);
    }
    luaL_error(L, "Assertion failed: %s\n", message);
  }
}
#else
void assert(bool condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
}
#endif /* if LUA_EXCEPTIONS */
