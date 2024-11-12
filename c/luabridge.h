#ifndef LUABRIDGE_H
#define LUABRIDGE_H
#include <lua.h>
#include <stdlib.h>

#define DEBUG_STR_BUF_LEN 1048576

typedef struct {
  const char **items;
  size_t count;
  size_t capacity;
} gplayback_keys;

#define check_usr_op(L, f, oidx)                                               \
  lua_pushstring(L, f);                                                        \
  lua_gettable(L, oidx);                                                       \
  if (!lua_isfunction(L, -1)) {                                                \
    luaL_error(L, "Expected function as " f " field");                         \
    return 0;                                                                  \
  }

#define call_usr_op(L, n)                                                      \
  if (lua_pcall(L, n, 1, 0) != 0) {                                            \
    printf("%s, skipping to next operation\n", lua_tostring(L, -1));           \
    entry = entry->next;                                                       \
    continue;                                                                  \
  }

int luaopen_playback(lua_State *L);

#endif // !LUABRIDGE_H
