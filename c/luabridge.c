#include <git2.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <unistd.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "luabridge.h"
#include "patch.h"
#include "segments.h"
#include "show.h"

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
    da_append((*keys), key);
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

int l_show_file_at_rev(lua_State *L) {
  set_err_lua_state(L);
  const char *file_path = luaL_checkstring(L, 1);
  const char *rev = luaL_checkstring(L, 2);
  gplayback_slice txt = show_file_at_rev(file_path, rev);
  lua_pushlstring(L, txt.ptr, txt.len);
  clear_err_lua_state();
  return 1;
}

int l_show_file_at_path(lua_State *L) {
  set_err_lua_state(L);
  const char *file_path = luaL_checkstring(L, 1);
  gplayback_slice txt = show_file_at_path(file_path);
  lua_pushlstring(L, txt.ptr, txt.len);
  clear_err_lua_state();
  return 1;
}

int l_get_diff_keys(lua_State *L) {
  set_err_lua_state(L);

  if (!lua_istable(L, 1)) {
    luaL_error(L, "Expected table as first argument");
    clear_err_lua_state();
    return 0;
  }

  lua_pushstring(L, "operations");
  lua_gettable(L, 1);
  int oidx = lua_gettop(L);

  if (!lua_istable(L, oidx)) {
    luaL_error(L, "Expected table as operations field");
    clear_err_lua_state();
    return 0;
  }

  const char *lhs_str = luaL_checkstring(L, 2);
  const char *rhs_str = luaL_checkstring(L, 3);

  gplayback_slice lhs = slice_from_string(lhs_str);
  gplayback_slice rhs = slice_from_string(rhs_str);

  gplayback_diff diff = diff_generate(lhs, rhs);
  gplayback_patch patch = patch_generate(&diff);

  check_usr_op(L, "insert_word_after", oidx);
  int ref_insert_word_after = lua_ref(L, true);

  check_usr_op(L, "insert_word_before", oidx);
  int ref_insert_word_before = lua_ref(L, true);

  check_usr_op(L, "insert_row_after", oidx);
  int ref_insert_row_after = lua_ref(L, true);

  check_usr_op(L, "insert_row_before", oidx);
  int ref_insert_row_before = lua_ref(L, true);

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

  gplayback_vm_operation_entry *entry = patch.first;
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
      char *src = slice_to_buf(data->src);
      lua_pushstring(L, src);
      call_usr_op(L, 3);
      free(src);
      break;
    }
    case GPLAYBACK_OP_INSERT_WORD_BEFORE: {
      gplayback_vm_op_insert_word_before *data = entry->item.data;
      lua_getref(L, ref_insert_word_before);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      char *src = slice_to_buf(data->src);
      lua_pushstring(L, src);
      call_usr_op(L, 3);
      free(src);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      lua_getref(L, ref_insert_row_after);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      char *src = slice_to_buf(data->src);
      lua_pushstring(L, src);
      call_usr_op(L, 3);
      free(src);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_BEFORE: {
      gplayback_vm_op_insert_row_before *data = entry->item.data;
      lua_getref(L, ref_insert_row_before);
      push_cursor(L, cursor);
      push_cursor(L, entry->item.cursor);
      char *src = slice_to_buf(data->src);
      lua_pushstring(L, src);
      call_usr_op(L, 3);
      free(src);
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
  }

  free(keys.items);
  patch_free(patch);
  diff_free(diff);

  clear_err_lua_state();
  return 1;
}

static const struct luaL_Reg playback[] = {
    {"showFileAtRev", l_show_file_at_rev},
    {"showFileAtPath", l_show_file_at_path},
    {"getDiffKeys", l_get_diff_keys},
    {NULL, NULL} // sentinel
};

int luaopen_playback(lua_State *L) {
  // Initialize libgit2
  git_libgit2_init();

  luaL_register(L, "playback", playback);
  return 1;
}
