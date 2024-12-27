#include <stdlib.h>

#ifdef LUA_OUT
#include <lua.h>
lua_State *lua_state = NULL;
void set_lua_state(lua_State *state) { lua_state = state; }
lua_State *get_lua_state() { return lua_state; }
void clear_lua_state() { lua_state = NULL; }
#else
void set_lua_state(void *state) {}
void *get_lua_state() { return NULL; }
void clear_lua_state() {}
#endif // !LUA_OUT
