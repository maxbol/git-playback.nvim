#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "escapestr.h"
#include "log.h"

void raw_error_f(const char *message) {
  fprintf(stderr, "%s\n", message);
  exit(1);
}

void pure_error_f(const char *message) {
  char escaped[OUT_BUFFER_SIZE] = {0};
  escape_string(message, escaped, OUT_BUFFER_SIZE);
  raw_error_f(escaped);
  exit(1);
}

#ifdef LUA_OUT
#include <lauxlib.h>
#include <lua.h>

#include "luastate.h"

void error_f(const char *message) {
  lua_State *lua_state = get_lua_state();
  if (lua_state == NULL) {
    pure_error_f(message);
  } else {
    luaL_error(lua_state, "%s\n", message);
  }
}
#else
void error_f(const char *message) { pure_error_f(message); }
#endif /* if LUA_OUT */
