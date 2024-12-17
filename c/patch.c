#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "diff.h"
#include "escapestr.h"
#include "flags.h"
#include "log.h"
#include "patch.h"
#include "slice.h"
#include "words.h"
#include "writestr.h"

gplayback_vm_operation_entry *
patch_append_operation_entry(gplayback_vm_operation_entry *entry, int type,
                             void *data, gplayback_cursorpos cursor) {
  // Split lines
  gplayback_vm_operation op = {
      type,
      data,
      cursor,
  };

  gplayback_vm_operation_entry *next_entry =
      malloc(sizeof(gplayback_vm_operation_entry));

  next_entry->item = op;
  next_entry->next = NULL;
  next_entry->prev = entry;

  if (entry != NULL) {
    entry->next = next_entry;
  }

  return next_entry;
}

// Inserts a copy of word *src before word *dest. Returns a pointer to the newly
// inserted word entry.
gplayback_vm_operation_entry *
patch_append_dl_operation_entry(gplayback_vm_operation_entry *entry,
                                deleteset *dl) {
  if (dl->lines_amount == 0) {
    return entry;
  }

  gplayback_vm_op_delete_rows *op_data =
      malloc(sizeof(gplayback_vm_op_delete_rows));

  op_data->no_of_lines = dl->lines_amount;

  gplayback_cursorpos cursor = {dl->anchor_line, 0};

  dbg_log("Adding DELETE_ROWS op: prev - %p; cursor - %d,%d", entry,
          cursor.line, cursor.column);

  dl->lines_amount = 0;
  dl->anchor_line = -1;

  return patch_append_operation_entry(entry, GPLAYBACK_OP_DELETE_ROWS, op_data,
                                      cursor);
}

/*gplayback_vm_operation_entry **/
/*patch_append_ml_operation_entry(gplayback_vm_operation_entry *entry,*/
/*                                gplayback_word_list_entry *nextline_anchor,*/
/*                                moveset *ml, gplayback_flags *visited) {*/
/*  if (ml->lines_amount == 0 || ml->move_amount == 0) {*/
/*    return entry;*/
/*  }*/
/**/
/*  assert(ml->anchor != NULL, "Move lines anchor is NULL");*/
/**/
/*  gplayback_vm_op_move_rows *op_data =*/
/*      malloc(sizeof(gplayback_vm_op_move_rows));*/
/**/
/*  op_data->no_of_lines = ml->lines_amount;*/
/*  // Relative move amount. ml->move_amount signifies the amount of lines
 * moved*/
/*  // from the top of the moveset if the moveset is 1 line long. The more
 * lines*/
/*  // are added to the moveset, the fewer steps need to be taken.*/
/*  // op_data->move_amount reflects this.*/
/*  op_data->move_amount = ml->move_amount - (ml->lines_amount - 1);*/
/**/
/*  gplayback_cursorpos cursor = {ml->anchor->item.line_idx,*/
/*                                ml->anchor->item.col_idx};*/
/**/
/*  gplayback_word_list_entry_refs refs = {0};*/
/**/
/*  gplayback_word_list_entry *line_anchor = ml->anchor;*/
/*  for (int i = 0; i < ml->lines_amount; i++) {*/
/*    assert(line_anchor != NULL, "Line anchor can't be NULL");*/
/**/
/*    int line_idx = line_anchor->item.line_idx;*/
/**/
/*    da_append(refs, line_anchor);*/
/**/
/*    // Move to next line anchor*/
/*    do {*/
/*      if (line_anchor->item.line_idx != line_idx) {*/
/*        break;*/
/*      }*/
/*    } while ((line_anchor = line_anchor->next));*/
/*  }*/
/**/
/*  for (int i = 0; i < refs.count; i++) {*/
/*    gplayback_word_list_entry *line_anchor = refs.items[i];*/
/**/
/*    word_move_words_absolute_until_eol(line_anchor, ml->move_amount);*/
/*    word_modifys_linenum_until_eol(line_anchor, op_data->move_amount);*/
/*    words_set_line_flag(visited, line_anchor, false);*/
/*  }*/
/**/
/*  free(refs.items);*/
/**/
/*  if (op_data->move_amount > 0) {*/
/*    assert(nextline_anchor != NULL, "Moveset nextline anchor has no next word
 * "*/
/*                                    "entry, this should not happen");*/
/*    words_modify_linenums(nextline_anchor, ml->move_amount,
 * -ml->lines_amount);*/
/*  } else {*/
/*    assert(ml->anchor->prev != NULL,*/
/*           "Moveset anchor has no previous word entry, this should not
 * happen");*/
/*    word_modify_words_linenum_backwards(ml->anchor->prev, ml->move_amount,*/
/*                                        ml->lines_amount);*/
/*  }*/
/**/
/*  dbg_log("Adding MOVE_ROWS op - prev: %p, cursorpos: %d,%d, moveamount:
 * %d",*/
/*          entry, cursor.line, cursor.column, ml->move_amount);*/
/**/
/*  ml->lines_amount = 0;*/
/*  ml->move_amount = 0;*/
/*  ml->anchor = NULL;*/
/**/
/*  return patch_append_operation_entry(entry, GPLAYBACK_OP_MOVE_ROWS,
 * op_data,*/
/*                                      cursor);*/
/*}*/
/**/
/*int calc_move_amount(gplayback_word_list_entry *rhs_anchor,*/
/*                     gplayback_word_list_entry *lhs_word_cursor) {*/
/*  int target_line = 0;*/
/*  gplayback_word_list_entry *rhs_anchor_rel = rhs_anchor->prev;*/
/**/
/*  while (rhs_anchor_rel != NULL) {*/
/*    if (rhs_anchor_rel->item.match != NULL) {*/
/*      gplayback_word_list_entry *lhs_anchor_rel =
 * rhs_anchor_rel->item.match;*/
/**/
/*      int anchor_distance =*/
/*          rhs_anchor->item.line_idx - rhs_anchor_rel->item.line_idx;*/
/**/
/*      target_line = lhs_anchor_rel->item.line_idx + anchor_distance;*/
/*      break;*/
/*    }*/
/*    rhs_anchor_rel = rhs_anchor_rel->prev;*/
/*  }*/
/**/
/*  if (target_line > lhs_word_cursor->item.line_idx) {*/
/*    target_line = target_line - 1;*/
/*  }*/
/**/
/*  return target_line - lhs_word_cursor->item.line_idx;*/
/*}*/
/**/
char *patch_debug(gplayback_patch *patch) {
  gplayback_writestr_state ws = writestr_create(512);

  writestr(ws, "Operations generated:\n");

  gplayback_vm_operation_entry *entry = patch->first;
  while (entry != NULL) {
    switch (entry->item.type) {
    case GPLAYBACK_OP_INSERT_WORD_AFTER: {
      gplayback_vm_op_insert_word_after *data = entry->item.data;
      char escaped_buf[512];
      unsigned int escaped_len =
          escape_fmt(escaped_buf, 512, " >> Insert word after [src=\"%.*s\"]",
                     (int)data->src.len, data->src.ptr);
      writestr(ws, "%.*s\n", escaped_len, escaped_buf);
      break;
    }
    case GPLAYBACK_OP_INSERT_WORD_BEFORE: {
      gplayback_vm_op_insert_word_before *data = entry->item.data;
      char escaped_buf[512];
      unsigned int escaped_len =
          escape_fmt(escaped_buf, 512, " >> Insert word before [src=\"%.*s\"]",
                     (int)data->src.len, data->src.ptr);
      writestr(ws, "%.*s\n", escaped_len, escaped_buf);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      char escaped_buf[512];
      unsigned int escaped_len =
          escape_fmt(escaped_buf, 512, " >> Insert row after [src=\"%.*s\"]",
                     (int)data->src.len, data->src.ptr);
      writestr(ws, "%.*s\n", escaped_len, escaped_buf);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_BEFORE: {
      gplayback_vm_op_insert_row_before *data = entry->item.data;
      char escaped_buf[512];
      unsigned int escaped_len =
          escape_fmt(escaped_buf, 512, " >> Insert row before [src=\"%.*s\"]",
                     (int)data->src.len, data->src.ptr);
      writestr(ws, "%.*s\n", escaped_len, escaped_buf);
      break;
    }
    case GPLAYBACK_OP_MOVE_ROWS: {
      gplayback_vm_op_move_rows *data = entry->item.data;
      writestr(ws, " >> Move rows [no_of_lines=%zu, move_amount=%d]\n",
               data->no_of_lines, data->move_amount);
      break;
    }
    case GPLAYBACK_OP_DELETE_ROWS: {
      gplayback_vm_op_delete_rows *data = entry->item.data;
      writestr(ws, " >> Delete rows [len=%zu]\n", data->no_of_lines);
      break;
    }
    case GPLAYBACK_OP_DELETE_WORDS: {
      gplayback_vm_op_delete_words *data = entry->item.data;
      writestr(ws, " >> Delete words [len=%zu]\n", data->char_len);
      break;
    }
    case GPLAYBACK_OP_CONCAT_ROWS: {
      writestr(ws, " >> Concat rows\n");
      break;
    }
    case GPLAYBACK_OP_SPLIT_ROWS: {
      writestr(ws, " >> Split rows\n");
      break;
    }
    case GPLAYBACK_OP_CUT_WORDS: {
      gplayback_vm_op_cut_words *data = entry->item.data;
      writestr(ws, " >> Cut words from [char_len=%zu]\n", data->char_len);
      break;
    }
    case GPLAYBACK_OP_PASTE_WORDS: {
      writestr(ws, " >> Paste words before\n");
      break;
    }
    }
    writestr(ws, " + Cursor: %d:%d\n\n", entry->item.cursor.line,
             entry->item.cursor.column);

    entry = entry->next;
  }

  return ws.out;
}

gplayback_vm_operation_entry *patch_handle_moveline(
    gplayback_word_list *lhs_words, gplayback_word_id *lhs_word_cursor,
    gplayback_diff_moveline moveline, gplayback_vm_operation_entry *entry) {
  gplayback_word_id cursor = moveline.lhs_anchor;

  gplayback_word_list_entry lhs_entry =
      words_get_entry(lhs_words, *lhs_word_cursor);
  gplayback_word_list_entry lhs_anchor_entry =
      words_get_entry(lhs_words, moveline.lhs_anchor);

  unsigned int move_amount =
      lhs_entry.item.line_idx - lhs_anchor_entry.item.line_idx;

  gplayback_vm_op_move_rows *op_data =
      malloc(sizeof(gplayback_vm_op_move_rows));

  op_data->move_amount = move_amount;
  op_data->no_of_lines = moveline.lines_amount;

  gplayback_cursorpos cursorpos = {lhs_anchor_entry.item.line_idx,
                                   lhs_anchor_entry.item.col_idx};

  dbg_log("Adding GPLAYBACK_OP_MOVE_ROWS op, cursorpos: %d, %d", cursorpos.line,
          cursorpos.column);

  entry = patch_append_operation_entry(entry, GPLAYBACK_OP_MOVE_ROWS, op_data,
                                       cursorpos);

  unsigned int i = moveline.lines_amount;
  while (cursor != 0 && i-- > 0) {
    words_move_line(lhs_words, cursor, *lhs_word_cursor, false);
    cursor = words_next(lhs_words, words_eol(lhs_words, cursor));
  }
  words_recalc_line_numbers(lhs_words);

  *lhs_word_cursor = moveline.lhs_anchor;

  return entry;
}

gplayback_vm_operation_entry *
patch_handle_splitlines(gplayback_word_list_entry rhs_entry,
                        gplayback_vm_operation_entry *entry) {

  gplayback_cursorpos cursor = {0, 0};
  cursor.line = rhs_entry.item.line_idx;

  dbg_log("Adding GPLAYBACK_OP_SPLIT_ROWS op, cursorpos: %d, %d", cursor.line,
          cursor.column);

  return patch_append_operation_entry(entry, GPLAYBACK_OP_SPLIT_ROWS, NULL,
                                      cursor);
}

gplayback_vm_operation_entry *patch_handle_insert_row(
    gplayback_word_list *rhs_words, gplayback_word_id *rhs_word_cursor,
    gplayback_word_id rhs_bol, gplayback_word_list_entry rhs_entry,
    gplayback_word_list *lhs_words, gplayback_word_id *lhs_word_cursor,
    gplayback_vm_operation_entry *entry, gplayback_flagset *flags) {
  // Insert entire line
  gplayback_cursorpos cursor = {0, 0};

  gplayback_vm_op_insert_row_after *op_data =
      malloc(sizeof(gplayback_vm_op_insert_row_after));

  gplayback_word_list_entry rhs_bol_entry = words_get_entry(rhs_words, rhs_bol);

  op_data->src = slice_subslice(rhs_bol_entry.item.ptr, 0,
                                words_line_charlen(rhs_words, rhs_bol));

  int opcode = GPLAYBACK_OP_INSERT_ROW_AFTER;
  char *opname = "GPLAYBACK_OP_INSERT_ROW_AFTER";

  gplayback_word_id lhs_prev = words_prev(lhs_words, *lhs_word_cursor);

  if (*lhs_word_cursor != 0 && lhs_prev != 0) {
    gplayback_word_list_entry prev = words_get_entry(lhs_words, lhs_prev);
    cursor.line = prev.item.line_idx;
  } else {
    opcode = GPLAYBACK_OP_INSERT_ROW_BEFORE;
    opname = "GPLAYBACK_OP_INSERT_ROW_BEFORE";
  }

  dbg_log("Adding %.*s op, src: %.*s, cursorpos: %d, %d", (int)strlen(opname),
          opname, (int)op_data->src.len, op_data->src.ptr, cursor.line,
          cursor.column);

  entry = patch_append_operation_entry(entry, opcode, op_data, cursor);

  if (*lhs_word_cursor != 0) {
    {
      char line_dump[4096];
      unsigned int line_len =
          words_print_line(line_dump, 4096, rhs_words, rhs_bol);
      dbg_log("@@@ Appending line %d: %.*s", rhs_entry.item.line_idx, line_len,
              line_dump);
      char words_dump[8192];
      unsigned int words_len =
          words_print_wordlist(words_dump, 8192, lhs_words);
      dbg_log("Text before:");
      dbg_log("%.*s", words_len, words_dump);
    }
    *lhs_word_cursor =
        words_copy_line(rhs_words, lhs_words, rhs_bol, *lhs_word_cursor);

    words_recalc_line_numbers(lhs_words);

    {
      char words_dump[8192];
      unsigned int words_len =
          words_print_wordlist(words_dump, 8192, lhs_words);
      dbg_log("Text after:");
      dbg_log("%.*s", words_len, words_dump);
    }

    flags_set_line(lhs_words, flags, *lhs_word_cursor,
                   FLAG_VISITED | FLAG_INSERT_PROCESSED, true);
  }

  // Fast-forward rhs cursor to last word on line
  *rhs_word_cursor = words_eol(rhs_words, *rhs_word_cursor);

  return entry;
}

gplayback_vm_operation_entry *patch_handle_insert_words(
    gplayback_word_list *rhs_words, gplayback_word_id *rhs_word_cursor,
    gplayback_word_list_entry rhs_entry, gplayback_word_list *lhs_words,
    gplayback_word_id *lhs_word_cursor, gplayback_vm_operation_entry *entry,
    gplayback_flagset *flags) {

  if (words_is_linesep(rhs_entry.item)) {
    return entry;
  }

  // Insert word
  gplayback_cursorpos cursor = {0, 0};

  gplayback_vm_op_insert_word_after *op_data =
      malloc(sizeof(gplayback_vm_op_insert_word_after));
  op_data->src = slice_subslice(rhs_entry.item.ptr, 0, rhs_entry.item.len);

  int opcode = GPLAYBACK_OP_INSERT_WORD_BEFORE;
  char *opname = "GPLAYBACK_OP_INSERT_WORD_BEFORE";

  if (*lhs_word_cursor == 0 ||
      *lhs_word_cursor == words_nextlt(lhs_words, *lhs_word_cursor)) {
    opcode = GPLAYBACK_OP_INSERT_WORD_AFTER;
    opname = "GPLAYBACK_OP_INSERT_WORD_AFTER";
  }

  if (*lhs_word_cursor != 0) {
    gplayback_word_list_entry lhs_entry =
        words_get_entry(lhs_words, *lhs_word_cursor);

    cursor.line = lhs_entry.item.line_idx;
    if (opcode == GPLAYBACK_OP_INSERT_WORD_AFTER) {
      cursor.column = lhs_entry.item.col_idx + lhs_entry.item.len - 1;
    } else {
      cursor.column = lhs_entry.item.col_idx;
    }
  }

  dbg_log("Adding %.*s op, cursorpos: %d, %d", (int)strlen(opname), opname,
          cursor.line, cursor.column);

  entry = patch_append_operation_entry(entry, opcode, op_data, cursor);

  if (*lhs_word_cursor != 0) {
    {
      char word_dump[1024];
      unsigned int word_len =
          words_print_word(word_dump, 1024, rhs_words, *rhs_word_cursor);
      dbg_log("@@@ Appending word %d: %.*s", *rhs_word_cursor, word_len,
              word_dump);
      char words_dump[WORD_LINE_MAX_LEN];
      unsigned int words_len = words_print_line(words_dump, WORD_LINE_MAX_LEN,
                                                lhs_words, *lhs_word_cursor);

      dbg_log("Line before:");
      dbg_log("%.*s", words_len, words_dump);
    }

    gplayback_word word_copy = rhs_entry.item;
    word_copy.match = 0;
    *lhs_word_cursor = words_insert(lhs_words, word_copy, *lhs_word_cursor);

    words_recalc_col_numbers(lhs_words, *lhs_word_cursor);

    {
      char words_dump[WORD_LINE_MAX_LEN];
      unsigned int words_len = words_print_line_with_highlights(
          words_dump, WORD_LINE_MAX_LEN, lhs_words, lhs_word_cursor, 1, "\e[0m",
          "\e[3;32m");

      dbg_log("Line after:");
      dbg_log("%.*s", words_len, words_dump);
    }

    flags_set(flags, *lhs_word_cursor, FLAG_VISITED | FLAG_INSERT_PROCESSED,
              true);

    *lhs_word_cursor = words_next(lhs_words, *lhs_word_cursor);
  }

  return entry;
}

gplayback_vm_operation_entry *patch_handle_movewords(
    gplayback_word_list *lhs_words, gplayback_word_id *lhs_word_cursor,
    gplayback_diff_movewords movewords, gplayback_vm_operation_entry *entry) {

  gplayback_word_id cursor = movewords.lhs_start;
  unsigned int i = movewords.words_amount;

  gplayback_word_list_entry *movewords_entry =
      words_get_entry_pointer(lhs_words, movewords.lhs_start);

  unsigned int char_len = 0;

  const unsigned int cut_line_idx = movewords_entry->item.line_idx;

  const gplayback_cursorpos cutpos = {.line = cut_line_idx,
                                      .column = movewords_entry->item.col_idx};

  while (cursor != 0 && i-- > 0) {
    if (cursor != *lhs_word_cursor) {
      gplayback_word_list_entry entry = words_get_entry(lhs_words, cursor);
      char_len += entry.item.len;

      words_move(lhs_words, cursor, *lhs_word_cursor, false);
    }
    cursor = words_next(lhs_words, cursor);
  }

  words_recalc_line_numbers(lhs_words);

  const unsigned int paste_line_idx = movewords_entry->item.line_idx;

  words_recalc_col_numbers(lhs_words, words_find_line(lhs_words, cut_line_idx));
  words_recalc_col_numbers(lhs_words,
                           words_find_line(lhs_words, paste_line_idx));

  gplayback_cursorpos pastepos = {.line = paste_line_idx,
                                  .column = movewords_entry->item.col_idx};

  gplayback_vm_op_cut_words *op_data =
      malloc(sizeof(gplayback_vm_op_cut_words));

  op_data->char_len = char_len;

  dbg_log("Adding GPLAYBACK_OP_CUT_WORDS op, cursorpos: %d, %d", cutpos.line,
          cutpos.column);

  entry = patch_append_operation_entry(entry, GPLAYBACK_OP_CUT_WORDS, op_data,
                                       cutpos);

  dbg_log("Adding GPLAYBACK_OP_PASTE_WORDS op, cursorpos: %d, %d",
          pastepos.line, pastepos.column);

  entry = patch_append_operation_entry(entry, GPLAYBACK_OP_PASTE_WORDS, NULL,
                                       pastepos);

  *lhs_word_cursor = movewords.lhs_start;

  return entry;
}

gplayback_vm_operation_entry *
patch_catch_up_rhs(gplayback_diff *diff, gplayback_word_id *lhs_word_cursor,
                   gplayback_word_id *rhs_word_cursor, gplayback_flagset *flags,
                   gplayback_vm_operation_entry *entry) {
  gplayback_word_list *lhs_words = &diff->lhs.words;
  gplayback_word_list *rhs_words = &diff->rhs.words;

  while (*rhs_word_cursor != 0) {
    gplayback_word_list_entry rhs_entry =
        words_get_entry(rhs_words, *rhs_word_cursor);

    dbg_log("Catching up RHS word %d: %.*s (current LHS cursor %d)",
            rhs_entry.item.word_id, (int)rhs_entry.item.len, rhs_entry.item.ptr,
            *lhs_word_cursor);

    gplayback_diff_moveline moveline = diff->movelines[*rhs_word_cursor];
    if (moveline.lhs_anchor != 0) {
      entry =
          patch_handle_moveline(lhs_words, lhs_word_cursor, moveline, entry);
      *rhs_word_cursor = words_next(rhs_words, *rhs_word_cursor);
      break;
    }

    gplayback_diff_movewords movewords = diff->movewords[*rhs_word_cursor];
    if (movewords.lhs_start != 0) {
      entry =
          patch_handle_movewords(lhs_words, lhs_word_cursor, movewords, entry);
      *rhs_word_cursor = words_next(rhs_words, *rhs_word_cursor);
      break;
    }

    if (rhs_entry.item.match != 0) {
      if (!flags_get(flags, rhs_entry.item.match, FLAG_INSERT_PROCESSED)) {
        // If the corresponding word on the LHS hasn't been visited
        // by the patch generator, then it is too early to insert any more
        // words from the RHS
        dbg_log("RHS word match is not visited: %.*s", (int)rhs_entry.item.len,
                rhs_entry.item.ptr);
        break;
      }
      dbg_log("RHS word is matched: %.*s", rhs_entry.item.len,
              rhs_entry.item.ptr);
      *rhs_word_cursor = words_next(rhs_words, *rhs_word_cursor);
      continue;
    }

    gplayback_word_id rhs_bol = words_bol(rhs_words, *rhs_word_cursor);

    if (words_nextlt(rhs_words, rhs_bol) == rhs_bol &&
        words_is_linesep(rhs_entry.item)) {

      entry = patch_handle_splitlines(rhs_entry, entry);

    } else if (!words_line_has_matches(rhs_words, rhs_bol)) {
      entry = patch_handle_insert_row(rhs_words, rhs_word_cursor, rhs_bol,
                                      rhs_entry, lhs_words, lhs_word_cursor,
                                      entry, flags);

    } else {
      entry =
          patch_handle_insert_words(rhs_words, rhs_word_cursor, rhs_entry,
                                    lhs_words, lhs_word_cursor, entry, flags);
    }

    *rhs_word_cursor = words_next(rhs_words, *rhs_word_cursor);
  }

  return entry;
}

void patch_free_operation_entries(gplayback_vm_operation_entry *entry) {
  if (entry == NULL) {
    return;
  }

  gplayback_vm_operation_entry *next = entry->next;

  free(entry->item.data);
  free(entry);

  patch_free_operation_entries(next);
}

void patch_free(gplayback_patch *patch) {
  patch_free_operation_entries(patch->first);
  diff_free(&patch->diff);
}

gplayback_patch patch_generate(gplayback_diff *diff) {
  gplayback_patch patch;
  patch.diff = diff_clone(diff);
  patch.first = NULL;

  gplayback_vm_operation_entry *entry = NULL;

  /*moveset ml = {0, 0, 0};*/
  deleteset dl = {-1, 0};

  gplayback_word_list *lhs_words = &patch.diff.lhs.words;
  gplayback_word_list *rhs_words = &patch.diff.rhs.words;

  /*gplayback_word_list_entry *lhs_anchor = NULL;*/
  /*gplayback_word_list_entry *rhs_anchor = NULL;*/
  gplayback_word_id lhs_word_cursor = lhs_words->first;
  gplayback_word_id rhs_word_cursor = rhs_words->first;

  gplayback_word_id last_lhs_word_cursor = lhs_word_cursor;

  gplayback_flagset flags = {0};

  bool line_is_dirty = false;

  while (lhs_word_cursor != 0) {
    gplayback_word_list_entry lhs_entry =
        words_get_entry(lhs_words, lhs_word_cursor);

    dbg_log("lhs_word_cursor: %d, %.*s", lhs_entry.item.word_id,
            (int)lhs_entry.item.len, lhs_entry.item.ptr);

    bool is_newline = false;

    if (lhs_word_cursor == words_prevlt(lhs_words, lhs_word_cursor)) {
      is_newline = true;
      line_is_dirty = words_line_has_matches(lhs_words, lhs_word_cursor);
    }

    if (is_newline) {
      if (line_is_dirty) {
        // This line should not be deleted, so we can commit all lines that are
        // marked for deletion into the DELETE ROWS operation
        if (dl.lines_amount > 0) {
          entry = patch_append_dl_operation_entry(entry, &dl);
        }
      } else {
        // This line should be deleted, and not moved, so we need to commit all
        // lines that are buffered for moving into the MOVE ROWS operation
        // before appending this line to the dl buffer
        /*if (ml.lines_amount > 0) {*/
        /*  entry = append_ml_operation_entry(entry, lhs_word_cursor, &ml,*/
        /*                                    &lhs_inserts_processed);*/
        /*}*/

        if (dl.anchor_line == -1) {
          dl.anchor_line = lhs_entry.item.line_idx;
          dl.lines_amount++;
        }

        lhs_word_cursor =
            words_delete_words_until_eol(lhs_words, lhs_word_cursor);

        words_recalc_line_numbers(lhs_words);

        continue;
      }
    }

    entry = patch_catch_up_rhs(&patch.diff, &lhs_word_cursor, &rhs_word_cursor,
                               &flags, entry);

    // lhs_word_cursor might have shifted, so we need to re-fetch the entry
    lhs_entry = words_get_entry(lhs_words, lhs_word_cursor);

    // Add DELETE_WORD, CONCAT_ROWS and SPLIT_ROWS operations as needed
    // But first check if visited (necessary because the word might have
    // have been moved here from elsewhere)
    if (!flags_get(&flags, lhs_word_cursor, FLAG_VISITED) &&
        lhs_entry.item.match == 0) {

      // Remove word
      gplayback_cursorpos cursor = {lhs_entry.item.line_idx,
                                    lhs_entry.item.col_idx};

      if (words_is_linesep(lhs_entry.item)) {

        // Concat lines
        entry = patch_append_operation_entry(entry, GPLAYBACK_OP_CONCAT_ROWS,
                                             NULL, cursor);
      } else {

        gplayback_vm_op_delete_words *op_data =
            malloc(sizeof(gplayback_vm_op_delete_words));

        op_data->char_len = lhs_entry.item.len;

        dbg_log("Adding DELETE_WORDS op, cursorpos: %d,%d", cursor.line,
                cursor.column);

        entry = patch_append_operation_entry(entry, GPLAYBACK_OP_DELETE_WORDS,
                                             op_data, cursor);
      }

      lhs_word_cursor = words_delete(lhs_words, lhs_word_cursor);

      if (lhs_word_cursor == 0) {
        break;
      }

      lhs_entry = words_get_entry(lhs_words, lhs_word_cursor);

      // Only recalc col numbers if the next word is on the same line
      if (lhs_entry.item.line_idx == cursor.line) {
        words_recalc_col_numbers(lhs_words, lhs_word_cursor);
      }
    }

    flags_set(&flags, lhs_word_cursor, FLAG_VISITED | FLAG_INSERT_PROCESSED,
              true);

    dbg_log("rhs_word_cursor: %d", rhs_word_cursor);
    if (rhs_word_cursor != 0) {
      gplayback_word_list_entry rhs_entry =
          words_get_entry(rhs_words, rhs_word_cursor);
      dbg_log("rhs_word: <<%.*s>>", (int)rhs_entry.item.len,
              rhs_entry.item.ptr);

      if (rhs_entry.item.match != 0) {
        gplayback_word_list_entry rhs_match =
            words_get_entry(lhs_words, rhs_entry.item.match);
        dbg_log("rhs_word match: <<%.*s>>", (int)rhs_match.item.len,
                rhs_match.item.ptr);
      }
    }

    last_lhs_word_cursor = lhs_word_cursor;
    lhs_word_cursor = words_next(lhs_words, lhs_word_cursor);
  }

  if (dl.lines_amount > 0) {
    entry = patch_append_dl_operation_entry(entry, &dl);
  }

  // Add a sentinel empty word to enable trailing RHS words to catch up to the
  // LHS cursor
  gplayback_word_list_entry *lhs_last_entry =
      words_get_entry_pointer(lhs_words, last_lhs_word_cursor);

  gplayback_word sentinel_word = (gplayback_word){
      .match = 0,
      .line_idx = 0,
      .len = 0,
      .ptr = "",
      .col_idx = 0,
  };

  if (words_is_linesep(lhs_last_entry->item)) {
    sentinel_word.line_idx = lhs_last_entry->item.line_idx + 1;
  } else {
    sentinel_word.line_idx = lhs_last_entry->item.line_idx;
    sentinel_word.col_idx =
        lhs_last_entry->item.col_idx + lhs_last_entry->item.len;
  }

  gplayback_word_id sentinel_word_id =
      words_create_entry(lhs_words, sentinel_word);
  gplayback_word_list_entry *sentinel_entry =
      words_get_entry_pointer(lhs_words, sentinel_word_id);

  sentinel_entry->prev = lhs_last_entry->item.word_id;

  if (lhs_last_entry != NULL) {
    lhs_last_entry->next = sentinel_entry->item.word_id;
  }

  // Catch up any remaining words on the RHS
  entry = patch_catch_up_rhs(&patch.diff, &sentinel_word_id, &rhs_word_cursor,
                             &flags, entry);

  // Reconstruct diff lhs entrypoint
  words_recalc_first(lhs_words);

  // Rewind the operation stack to the beginning
  if (entry != NULL) {
    while (entry->prev != NULL) {
      entry = entry->prev;
    }
  }

  patch.first = entry;

  return patch;
}
