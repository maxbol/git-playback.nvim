#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <unistd.h>

#include "patch.c"

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

void push_cursor(lua_State *L, gplayback_cursorpos cursor) {
  lua_newtable(L);
  lua_pushstring(L, "line");
  lua_pushinteger(L, cursor.line);
  lua_settable(L, -3);
  lua_pushstring(L, "column");
  lua_pushinteger(L, cursor.column);
  lua_settable(L, -3);
}

void unpack_result(lua_State *L, gplayback_cursorpos *cursor,
                   gplayback_keys *keys) {
  if (!lua_istable(L, -1)) {
    luaL_error(L, "Expected table as return value");
    return;
  }

  int result_idx = lua_gettop(L);

  lua_pushstring(L, "keys");
  lua_gettable(L, result_idx);

  int keys_idx = lua_gettop(L);

  if (!lua_istable(L, keys_idx)) {
    luaL_error(L, "Expected table as keys field");
    return;
  }

  int keys_len = luaL_getn(L, keys_idx);

  for (int i = 1; i <= keys_len; i++) {
    lua_rawgeti(L, keys_idx, i);
    const char *key = lua_tostring(L, -1);
    da_append_ptr(keys, key);
  }

  lua_pushstring(L, "cursor");
  lua_gettable(L, result_idx);

  if (!lua_istable(L, -1)) {
    luaL_error(L, "Expected table as cursor field");
    return;
  }

  int cursor_idx = lua_gettop(L);

  lua_pushstring(L, "line");
  lua_gettable(L, cursor_idx);
  cursor->line = lua_tointeger(L, -1);

  lua_pushstring(L, "column");
  lua_gettable(L, cursor_idx);
  cursor->column = lua_tointeger(L, -1);
}

int l_generate_diff(lua_State *L) {
  const char *lhs_str = luaL_checkstring(L, 1);
  const char *rhs_str = luaL_checkstring(L, 2);
  gplayback_slice lhs = strslice(lhs_str);
  gplayback_slice rhs = strslice(rhs_str);
  gplayback_diff *diff = lua_newuserdata(L, sizeof(gplayback_diff));
  *diff = generate_diff(lhs, rhs);
  return 1;
}

int l_generate_patch(lua_State *L) {
  gplayback_diff *diff = lua_touserdata(L, 1);
  gplayback_patch *patch = lua_newuserdata(L, sizeof(gplayback_patch));
  *patch = generate_patch(*diff);
  return 1;
}

int l_debugprint_diff(lua_State *L) {
  gplayback_diff *diff = lua_touserdata(L, 1);
  debug_diff(*diff);
  return 0;
}

int l_debugprint_patch(lua_State *L) {
  gplayback_patch *patch = lua_touserdata(L, 1);
  debug_patch(*patch);
  return 0;
}

int l_get_patch_keys(lua_State *L) {
  if (!lua_istable(L, 1)) {
    luaL_error(L, "Expected table as first argument");
    return 0;
  }

  gplayback_patch *patch = lua_touserdata(L, 2);

  lua_pushstring(L, "operations");
  lua_gettable(L, 1);
  int oidx = lua_gettop(L);

  if (!lua_istable(L, oidx)) {
    luaL_error(L, "Expected table as operations field");
    return 0;
  }

  check_usr_op(L, "insert_word_after", oidx);
  int ref_insert_word_after = lua_ref(L, true);

  check_usr_op(L, "insert_row_after", oidx);
  int ref_insert_row_after = lua_ref(L, true);

  check_usr_op(L, "delete_words", oidx);
  int delete_words_ref = lua_ref(L, true);

  check_usr_op(L, "delete_rows", oidx);
  int delete_rows_ref = lua_ref(L, true);

  check_usr_op(L, "move_rows", oidx);
  int move_rows_ref = lua_ref(L, true);

  check_usr_op(L, "concat_rows", oidx);
  int concat_rows_ref = lua_ref(L, true);

  check_usr_op(L, "split_rows", oidx);
  int split_rows_ref = lua_ref(L, true);

  check_usr_op(L, "goto_position", oidx);
  int goto_position_ref = lua_ref(L, true);

  gplayback_vm_operation_entry *entry = patch->first;
  gplayback_cursorpos cursor = {0};
  gplayback_keys keys = {0};

  while (entry != NULL) {
    if (cursor.line != entry->item.cursor.line ||
        cursor.column != entry->item.cursor.column) {

      lua_getref(L, goto_position_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      call_usr_op(L, 2);
      unpack_result(L, &cursor, &keys);
      continue;
    }

    switch (entry->item.type) {
    case GPLAYBACK_OP_INSERT_WORD_AFTER: {
      gplayback_vm_op_insert_word_after *data = entry->item.data;
      lua_getref(L, ref_insert_word_after);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      lua_pushstring(L, slice_to_buf(data->src));
      call_usr_op(L, 3);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      lua_getref(L, ref_insert_row_after);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      lua_pushstring(L, slice_to_buf(data->src));
      call_usr_op(L, 3);
      break;
    }
    case GPLAYBACK_OP_MOVE_ROWS: {
      gplayback_vm_op_move_rows *data = entry->item.data;
      lua_getref(L, move_rows_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      lua_pushinteger(L, data->no_of_lines);
      lua_pushinteger(L, data->move_amount);
      call_usr_op(L, 4);
      break;
    }
    case GPLAYBACK_OP_DELETE_ROWS: {
      gplayback_vm_op_delete_rows *data = entry->item.data;
      lua_getref(L, delete_rows_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      lua_pushinteger(L, data->no_of_lines);
      call_usr_op(L, 3);
      break;
    }
    case GPLAYBACK_OP_DELETE_WORDS: {
      gplayback_vm_op_delete_words *data = entry->item.data;
      lua_getref(L, delete_words_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      lua_pushinteger(L, data->char_len);
      call_usr_op(L, 3);
      break;
    }
    case GPLAYBACK_OP_CONCAT_ROWS: {
      lua_getref(L, concat_rows_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      call_usr_op(L, 2);
      break;
    }
    case GPLAYBACK_OP_SPLIT_ROWS: {
      lua_getref(L, split_rows_ref);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      call_usr_op(L, 2);
      break;
    }
    }

    unpack_result(L, &cursor, &keys);

    entry = entry->next;
  }

  lua_newtable(L);
  int keys_idx = lua_gettop(L);
  for (size_t i = 0; i < keys.count; i++) {
    lua_pushstring(L, keys.items[i]);
    lua_rawseti(L, keys_idx, i + 1);
    /*free(keys.items[i]);*/
  }

  free(keys.items);

  return 1;
}

static int l_sleep(lua_State *L) {
  int m = luaL_checknumber(L, 1);
  usleep(m * 1000);
  return 0;
}

static const struct luaL_Reg playback[] = {
    {"generateDiff", l_generate_diff},
    {"generatePatch", l_generate_patch},
    {"debugprintDiff", l_debugprint_diff},
    {"debugprintPatch", l_debugprint_patch},
    {"getPatchKeys", l_get_patch_keys},
    {"sleep", l_sleep},
    {NULL, NULL} // sentinel
};

int luaopen_playback(lua_State *L) {
  luaL_register(L, "playback", playback);
  return 1;
}
