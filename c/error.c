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
  char escaped[ESCAPE_BUFFER_SIZE] = {0};
  escape_string(message, escaped, ESCAPE_BUFFER_SIZE);
  raw_error_f(escaped);
  exit(1);
}

#ifdef LUA_EXCEPTIONS
#include <lauxlib.h>
#include <lua.h>

lua_State *lua_state = NULL;

void set_err_lua_state(lua_State *state) { lua_state = state; }
void clear_err_lua_state() { lua_state = NULL; }

void error_f(const char *message) {
  if (lua_state == NULL) {
    pure_error_f(message);
  } else {
    luaL_error(lua_state, "%s\n", message);
  }
}
#else
void set_err_lua_state(void *state) {}
void clear_err_lua_state() {}
void error_f(const char *message) { pure_error_f(message); }
#endif /* if LUA_EXCEPTIONS */
