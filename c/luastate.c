#include <stdlib.h>

#ifdef LUA_OUT
#include <lua.h>
lua_State *lua_state = NULL;
void clear_lua_state() { lua_state = NULL; }
lua_State *get_lua_state() { return lua_state; }
void set_lua_state(lua_State *state) { lua_state = state; }
#else
void clear_lua_state() {}
void *get_lua_state() { return NULL; }
void set_lua_state(void *state) {}
#endif // !LUA_OUT
