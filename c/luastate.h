#ifndef LUASTATE_H
#define LUASTATE_H
#include <stdio.h>

#ifdef LUAOUT
#include <lua.h>
typedef lua_State *log_lua_state;
#else
typedef void *log_lua_state;
#endif // !LUAOUT

void set_lua_state(log_lua_state state);
log_lua_state get_lua_state();
void clear_lua_state();

#endif /* LUASTATE_H */
