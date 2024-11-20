#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "diff.h"
#include "log.h"
#include "patch.h"
#include "words.h"
#include "writestr.h"

gplayback_vm_operation_entry *
append_operation_entry(gplayback_vm_operation_entry *entry, int type,
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
append_dl_operation_entry(gplayback_vm_operation_entry *entry, deleteset *dl) {
  if (dl->lines_amount == 0) {
    return entry;
  }

  gplayback_vm_op_delete_rows *op_data =
      malloc(sizeof(gplayback_vm_op_delete_rows));

  op_data->no_of_lines = dl->lines_amount;

  gplayback_cursorpos cursor = {dl->anchor_line, 0};

  dbg_log("Adding DELETE_ROWS op: prev - %p; cursor - %d,%d\n", entry,
          cursor.line, cursor.column);

  dl->lines_amount = 0;
  dl->anchor_line = -1;

  return append_operation_entry(entry, GPLAYBACK_OP_DELETE_ROWS, op_data,
                                cursor);
}

gplayback_vm_operation_entry *
append_ml_operation_entry(gplayback_vm_operation_entry *entry,
                          gplayback_word_list_entry *nextline_anchor,
                          moveset *ml, gplayback_flags *visited) {
  if (ml->lines_amount == 0 || ml->move_amount == 0) {
    return entry;
  }

  assert(ml->anchor != NULL, "Move lines anchor is NULL");

  gplayback_vm_op_move_rows *op_data =
      malloc(sizeof(gplayback_vm_op_move_rows));

  op_data->no_of_lines = ml->lines_amount;
  // Relative move amount. ml->move_amount signifies the amount of lines moved
  // from the top of the moveset if the moveset is 1 line long. The more lines
  // are added to the moveset, the fewer steps need to be taken.
  // op_data->move_amount reflects this.
  op_data->move_amount = ml->move_amount - (ml->lines_amount - 1);

  gplayback_cursorpos cursor = {ml->anchor->item.line_idx,
                                ml->anchor->item.col_idx};

  gplayback_word_list_entry_refs refs = {0};

  gplayback_word_list_entry *line_anchor = ml->anchor;
  for (int i = 0; i < ml->lines_amount; i++) {
    assert(line_anchor != NULL, "Line anchor can't be NULL");

    int line_idx = line_anchor->item.line_idx;

    da_append(refs, line_anchor);

    // Move to next line anchor
    do {
      if (line_anchor->item.line_idx != line_idx) {
        break;
      }
      /*assert(line_anchor->next != NULL, "Moveset line anchor has no next word
       * "*/
      /*                                  "entry, this should not happen");*/
    } while ((line_anchor = line_anchor->next));
  }

  for (int i = 0; i < refs.count; i++) {
    gplayback_word_list_entry *line_anchor = refs.items[i];

    move_words_absolute_until_eol(line_anchor, ml->move_amount);
    modify_words_linenum_until_eol(line_anchor, op_data->move_amount);
    mark_words_visited_until_eol(visited, line_anchor, false);
  }

  free(refs.items);

  if (op_data->move_amount > 0) {
    assert(nextline_anchor != NULL, "Moveset nextline anchor has no next word "
                                    "entry, this should not happen");
    modify_words_linenum(nextline_anchor, ml->move_amount, -ml->lines_amount);
  } else {
    assert(ml->anchor->prev != NULL,
           "Moveset anchor has no previous word entry, this should not happen");
    modify_words_linenum_backwards(ml->anchor->prev, ml->move_amount,
                                   ml->lines_amount);
  }

  dbg_log("Adding MOVE_ROWS op - prev: %p, cursorpos: %d,%d, moveamount: %d\n",
          entry, cursor.line, cursor.column, ml->move_amount);

  ml->lines_amount = 0;
  ml->move_amount = 0;
  ml->anchor = NULL;

  return append_operation_entry(entry, GPLAYBACK_OP_MOVE_ROWS, op_data, cursor);
}

int calc_move_amount(gplayback_word_list_entry *rhs_anchor,
                     gplayback_word_list_entry *lhs_word_cursor) {
  int target_line = 0;
  gplayback_word_list_entry *rhs_anchor_rel = rhs_anchor->prev;

  while (rhs_anchor_rel != NULL) {
    if (rhs_anchor_rel->item.match != NULL) {
      gplayback_word_list_entry *lhs_anchor_rel = rhs_anchor_rel->item.match;

      int anchor_distance =
          rhs_anchor->item.line_idx - rhs_anchor_rel->item.line_idx;

      target_line = lhs_anchor_rel->item.line_idx + anchor_distance;
      break;
    }
    rhs_anchor_rel = rhs_anchor_rel->prev;
  }

  if (target_line > lhs_word_cursor->item.line_idx) {
    target_line = target_line - 1;
  }

  return target_line - lhs_word_cursor->item.line_idx;
}

char *debug_patch(gplayback_patch patch) {
  char *out = malloc(512);
  size_t offset = 0;
  size_t capacity = 512;

  memset(out, 0, 512);

#define w(...) writestr(out, offset, capacity, __VA_ARGS__)

  w("Operations generated:\n");

  gplayback_vm_operation_entry *entry = patch.first;
  while (entry != NULL) {
    switch (entry->item.type) {
    case GPLAYBACK_OP_INSERT_WORD_AFTER: {
      gplayback_vm_op_insert_word_after *data = entry->item.data;
      w(" >> Insert word after [src=\"%.*s\"]\n", (int)data->src.len,
        data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_INSERT_WORD_BEFORE: {
      gplayback_vm_op_insert_word_before *data = entry->item.data;
      w(" >> Insert word before [src=\"%.*s\"]\n", (int)data->src.len,
        data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      w(" >> Insert row after [src=\"%.*s\"]\n", (int)data->src.len,
        data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_BEFORE: {
      gplayback_vm_op_insert_row_before *data = entry->item.data;
      w(" >> Insert row before [src=\"%.*s\"]\n", (int)data->src.len,
        data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_MOVE_ROWS: {
      gplayback_vm_op_move_rows *data = entry->item.data;
      w(" >> Move rows [no_of_lines=%zu, move_amount=%d]\n", data->no_of_lines,
        data->move_amount);
      break;
    }
    case GPLAYBACK_OP_DELETE_ROWS: {
      gplayback_vm_op_delete_rows *data = entry->item.data;
      w(" >> Delete rows [len=%zu]\n", data->no_of_lines);
      break;
    }
    case GPLAYBACK_OP_DELETE_WORDS: {
      gplayback_vm_op_delete_words *data = entry->item.data;
      w(" >> Delete words [len=%zu]\n", data->char_len);
      break;
    }
    case GPLAYBACK_OP_CONCAT_ROWS: {
      w(" >> Concat rows\n");
      break;
    }
    case GPLAYBACK_OP_SPLIT_ROWS: {
      w(" >> Split rows\n");
      break;
    }
    }
    w(" + Cursor: %d:%d\n\n", entry->item.cursor.line,
      entry->item.cursor.column);

    entry = entry->next;
  }

  return out;
}

gplayback_vm_operation_entry *
catch_up_rhs(gplayback_word_list_entry **lhs_word_cursor,
             gplayback_word_list_entry **rhs_word_cursor,
             gplayback_flags *lhs_inserts_processed,
             gplayback_flags *lhs_visited, gplayback_diff diff,
             gplayback_vm_operation_entry *entry) {
  do {
    if (*rhs_word_cursor == NULL) {
      break;
    }
    dbg_log("Catching up RHS word: %.*s\n", (int)(*rhs_word_cursor)->item.len,
            (*rhs_word_cursor)->item.ptr);

    if ((*rhs_word_cursor)->item.match != NULL) {
      if (!is_word_visited(*lhs_inserts_processed,
                           *(*rhs_word_cursor)->item.match)) {
        // If the corresponding word on the LHS hasn't been visited
        // by the patch generator, then it is too early to insert any more
        // words from the RHS
        dbg_log("RHS word match is not visited: %.*s\n",
                (int)(*rhs_word_cursor)->item.len,
                (*rhs_word_cursor)->item.ptr);
        break;
      }
      dbg_log("RHS word is matched: %.*s\n", (int)(*rhs_word_cursor)->item.len,
              (*rhs_word_cursor)->item.ptr);
      continue;
    }

    gplayback_line rhs_line =
        diff.rhs.lines.items[(*rhs_word_cursor)->item.line_idx];

    if (!rhs_line.dirty) {
      // Insert entire line
      gplayback_cursorpos cursor = {0, 0};

      gplayback_vm_op_insert_row_after *op_data =
          malloc(sizeof(gplayback_vm_op_insert_row_after));

      op_data->src = subslice(rhs_line.words->item.ptr, 0, rhs_line.char_len);

      int opcode = GPLAYBACK_OP_INSERT_ROW_AFTER;
      char *opname = "GPLAYBACK_OP_INSERT_ROW_AFTER";

      if (*lhs_word_cursor != NULL && (*lhs_word_cursor)->prev != NULL) {
        cursor.line = (*lhs_word_cursor)->prev->item.line_idx;
      } else {
        opcode = GPLAYBACK_OP_INSERT_ROW_BEFORE;
        opname = "GPLAYBACK_OP_INSERT_ROW_BEFORE";
      }

      dbg_log("Adding %.*s op, cursorpos: %d, %d\n", (int)strlen(opname),
              opname, cursor.line, cursor.column);

      entry = append_operation_entry(entry, opcode, op_data, cursor);

      if (*lhs_word_cursor != NULL) {
        modify_words_linenum(*lhs_word_cursor, -1, 1);

        *lhs_word_cursor =
            insert_words_copy_until_eol(*rhs_word_cursor, *lhs_word_cursor);

        mark_words_visited_until_eol(lhs_visited, bol((*lhs_word_cursor)->prev),
                                     true);
        mark_words_visited_until_eol(lhs_inserts_processed,
                                     bol((*lhs_word_cursor)->prev), true);
      }

      // Fast-forward rhs cursor to last word on line
      do {
        if ((*rhs_word_cursor)->next == NULL ||
            ((*rhs_word_cursor)->item.line_idx !=
             (*rhs_word_cursor)->next->item.line_idx)) {
          break;
        }
      } while ((*rhs_word_cursor = (*rhs_word_cursor)->next));
    } else {
      // Insert word
      gplayback_cursorpos cursor = {0, 0};

      gplayback_vm_op_insert_word_after *op_data =
          malloc(sizeof(gplayback_vm_op_insert_word_after));

      op_data->src = subslice((*rhs_word_cursor)->item.ptr, 0,
                              (*rhs_word_cursor)->item.len);

      int opcode = GPLAYBACK_OP_INSERT_WORD_BEFORE;
      char *opname = "GPLAYBACK_OP_INSERT_WORD_BEFORE";

      bool is_last = *lhs_word_cursor == linenext(*lhs_word_cursor);

      if (*lhs_word_cursor == NULL || is_last) {
        opcode = GPLAYBACK_OP_INSERT_WORD_AFTER;
        opname = "GPLAYBACK_OP_INSERT_WORD_AFTER";
      }

      if (*lhs_word_cursor != NULL) {
        cursor.line = (*lhs_word_cursor)->item.line_idx;
        if (opcode == GPLAYBACK_OP_INSERT_WORD_AFTER) {
          cursor.column = (*lhs_word_cursor)->item.col_idx +
                          (*lhs_word_cursor)->item.len - 1;
        } else {
          cursor.column = (*lhs_word_cursor)->item.col_idx;
        }
      }

      dbg_log("Adding %.*s op, cursorpos: %d, %d\n", (int)strlen(opname),
              opname, cursor.line, cursor.column);

      entry = append_operation_entry(entry, opcode, op_data, cursor);

      if (*lhs_word_cursor != NULL) {
        modify_words_colnum_until_eol(*lhs_word_cursor,
                                      (*rhs_word_cursor)->item.len);

        dbg_log("%d\n", (*lhs_word_cursor)->item.word_id);
        *lhs_word_cursor = insert_word_copy(*rhs_word_cursor, *lhs_word_cursor);

        mark_word_visited(lhs_visited, *lhs_word_cursor, true);
        mark_word_visited(lhs_inserts_processed, *lhs_word_cursor, true);

        *lhs_word_cursor = (*lhs_word_cursor)->next;
      }
    }
  } while ((*rhs_word_cursor = (*rhs_word_cursor)->next));

  return entry;
}

void free_operation_entry(gplayback_vm_operation_entry *entry) {
  if (entry == NULL) {
    return;
  }

  gplayback_vm_operation_entry *next = entry->next;
  free(entry->item.data);
  free(entry);
  free_operation_entry(next);
}

void free_patch(gplayback_patch patch) { free_operation_entry(patch.first); }

gplayback_patch generate_patch(gplayback_diff *diff) {
  gplayback_vm_operation_entry *entry = NULL;

  moveset ml = {0, 0, 0};
  deleteset dl = {-1, 0};

  gplayback_word_list_entry *lhs_anchor = NULL;
  gplayback_word_list_entry *rhs_anchor = NULL;
  gplayback_word_list_entry *lhs_word_cursor = diff->lhs.words.first;
  gplayback_word_list_entry *rhs_word_cursor = diff->rhs.words.first;

  gplayback_word_list_entry *last_lhs_word_cursor = lhs_word_cursor;

  gplayback_flags lhs_visited = {0};
  gplayback_flags lhs_inserts_processed = {0};

  int last_wordid = get_last_wordid(lhs_word_cursor);

  da_capacity(lhs_visited, last_wordid);
  da_capacity(lhs_inserts_processed, last_wordid);

  bool line_is_dirty = false;

  do {
    if (lhs_word_cursor == NULL) {
      break;
    }
    dbg_log("lhs_word_cursor: %d, %.*s\n", lhs_word_cursor->item.word_id,
            (int)lhs_word_cursor->item.len, lhs_word_cursor->item.ptr);

    bool is_newline = false;

    if (lhs_word_cursor->prev == NULL ||
        lhs_word_cursor->item.line_idx !=
            lhs_word_cursor->prev->item.line_idx) {
      /*line = &diff.lhs.lines.items[lhs_word_cursor->item.line_idx];*/
      is_newline = true;
      lhs_anchor = NULL;
      rhs_anchor = NULL;

      line_is_dirty = is_dirty_line(lhs_word_cursor);
    }

    if (is_newline) {
      if (line_is_dirty) {
        // This line should not be deleted, so we can commit all lines that are
        // marked for deletion into the DELETE ROWS operation
        if (dl.lines_amount > 0) {
          entry = append_dl_operation_entry(entry, &dl);
        }
      } else {
        // This line should be deleted, and not moved, so we need to commit all
        // lines that are buffered for moving into the MOVE ROWS operation
        // before appending this line to the dl buffer
        if (ml.lines_amount > 0) {
          entry = append_ml_operation_entry(entry, lhs_word_cursor, &ml,
                                            &lhs_inserts_processed);
        }

        if (dl.anchor_line == -1) {
          dl.anchor_line = lhs_word_cursor->item.line_idx;
          dl.lines_amount++;
        }

        // We shouldn't need to mark words that are to be deleted as visited,
        // right?
        /*mark_words_visited_until_eol(&lhs_visited, lhs_word_cursor, true);*/

        lhs_word_cursor = delete_words_until_eol(lhs_word_cursor);

        if (lhs_word_cursor != NULL) {
          modify_words_linenum(lhs_word_cursor, -1, -1);
        }

        continue;
      }
    }

    // Have the rhs cursor catch up to our position, inserting
    // words/lines as we go
    entry = catch_up_rhs(&lhs_word_cursor, &rhs_word_cursor,
                         &lhs_inserts_processed, &lhs_visited, *diff, entry);

    // Add DELETE_WORD, CONCAT_ROWS and SPLIT_ROWS operations as needed
    // But first check if visited (necessary because the word might have
    // have been moved here from elsewhere)
    if (!is_word_visited(lhs_visited, *lhs_word_cursor)) {
      if (lhs_word_cursor->item.match == NULL) {
        // Remove word
        gplayback_cursorpos cursor = {lhs_word_cursor->item.line_idx,
                                      lhs_word_cursor->item.col_idx};

        gplayback_vm_op_delete_words *op_data =
            malloc(sizeof(gplayback_vm_op_delete_words));

        op_data->char_len = 0;
        for (int i = 0; i < lhs_word_cursor->item.len; i++) {
          if (lhs_word_cursor->item.ptr[i] == GPLAYBACK_TOKEN_NEWLINE) {
            break;
          }
          op_data->char_len++;
        }

        dbg_log("Adding DELETE_WORDS op, cursorpos: %d,%d\n", cursor.line,
                cursor.column);

        entry = append_operation_entry(entry, GPLAYBACK_OP_DELETE_WORDS,
                                       op_data, cursor);

        // This should not be necessary, commenting out...
        /*mark_word_visited(&lhs_visited, lhs_word_cursor, true);*/

        lhs_word_cursor = delete_word(lhs_word_cursor);

        if (lhs_word_cursor == NULL) {
          break;
        }

        modify_words_colnum_until_eol(lhs_word_cursor, -op_data->char_len);

        continue;
      } else {
        gplayback_word_list_entry *match_word = lhs_word_cursor->item.match;

        if (lhs_anchor == NULL) {
          lhs_anchor = lhs_word_cursor;
          rhs_anchor = match_word;
        }

        gplayback_word_list_entry *lhs_next = lhs_word_cursor->next;

        if (lhs_next != NULL && match_word->next != NULL &&
            lhs_next->item.match == match_word->next) {
          char lhs_lastchar =
              lhs_word_cursor->item.ptr[lhs_word_cursor->item.len - 1];
          char rhs_lastchar = match_word->item.ptr[match_word->item.len - 1];

          gplayback_cursorpos cursor = {lhs_word_cursor->item.line_idx,
                                        lhs_word_cursor->item.col_idx +
                                            lhs_word_cursor->item.len - 1};

          if (lhs_lastchar == GPLAYBACK_TOKEN_NEWLINE &&
              rhs_lastchar != GPLAYBACK_TOKEN_NEWLINE) {
            // Concat lines
            entry = append_operation_entry(entry, GPLAYBACK_OP_CONCAT_ROWS,
                                           NULL, cursor);
            lhs_word_cursor->item.ptr = match_word->item.ptr;
            lhs_word_cursor->item.len = match_word->item.len;
            modify_words_linenum(lhs_next, -1, -1);
            modify_words_colnum_until_eol(lhs_next,
                                          lhs_word_cursor->item.col_idx +
                                              lhs_word_cursor->item.len);
          } else if (lhs_lastchar != GPLAYBACK_TOKEN_NEWLINE &&
                     rhs_lastchar == GPLAYBACK_TOKEN_NEWLINE) {
            // Split lines
            entry = append_operation_entry(entry, GPLAYBACK_OP_SPLIT_ROWS, NULL,
                                           cursor);
            lhs_word_cursor->item.ptr = match_word->item.ptr;
            lhs_word_cursor->item.len = match_word->item.len;
            modify_words_linenum(lhs_next, -1, 1);
            modify_words_colnum_until_eol(
                lhs_next,
                -(lhs_word_cursor->item.col_idx + lhs_word_cursor->item.len));
          }
        }
      }
    }

    if (!is_word_visited(lhs_visited, *lhs_word_cursor) &&
        (lhs_word_cursor->next == NULL ||
         lhs_word_cursor->next->item.line_idx !=
             lhs_word_cursor->item.line_idx)) {

      // End of line, see if line should be moved

      if (lhs_anchor != NULL && rhs_anchor != NULL &&
          lhs_anchor->item.line_idx != rhs_anchor->item.line_idx) {
        int move_amount = calc_move_amount(rhs_anchor, lhs_word_cursor);

        if (move_amount != 0) {
          if (ml.lines_amount > 0) {
            entry = append_ml_operation_entry(entry, lhs_anchor, &ml,
                                              &lhs_inserts_processed);

            // We have to redo the calculation here, because the positions might
            // have shifted around with the move
            move_amount = calc_move_amount(rhs_anchor, lhs_word_cursor);
          }
          ml.anchor = lhs_anchor;
          ml.move_amount = move_amount;
        }

        ml.lines_amount++;
      } else if (ml.lines_amount > 0) {
        entry = append_ml_operation_entry(entry, lhs_anchor, &ml,
                                          &lhs_inserts_processed);
      }
    }

    // Mark the LHS word as visited
    mark_word_visited(&lhs_visited, lhs_word_cursor, true);
    mark_word_visited(&lhs_inserts_processed, lhs_word_cursor, true);

    last_lhs_word_cursor = lhs_word_cursor;
    lhs_word_cursor = lhs_word_cursor->next;
  } while (lhs_word_cursor != NULL);

  if (ml.lines_amount > 0) {
    // TODO(2024-10-28, Max Bolotin): This will always break the assertion in
    // append_ml_operation_entry, what is expected behaviour here?
    entry = append_ml_operation_entry(entry, NULL, &ml, &lhs_inserts_processed);
  }

  if (dl.lines_amount > 0) {
    entry = append_dl_operation_entry(entry, &dl);
  }

  // UGLY!!!
  // This can probably be done in a more efficient way
  char *empty = "";
  gplayback_word_list_entry *lhs_word_cursor_sentinel =
      malloc(sizeof(gplayback_word_list_entry));
  lhs_word_cursor_sentinel->item = (gplayback_word){empty, NULL, 0, 0, 0, 0};
  lhs_word_cursor_sentinel->prev = last_lhs_word_cursor;
  lhs_word_cursor_sentinel->item.line_idx = last_lhs_word_cursor->item.line_idx;

  if (last_lhs_word_cursor != NULL) {
    last_lhs_word_cursor->next = lhs_word_cursor_sentinel;
  }

  // Catch up any remaining words on the RHS
  entry = catch_up_rhs(&lhs_word_cursor_sentinel, &rhs_word_cursor,
                       &lhs_inserts_processed, &lhs_visited, *diff, entry);

  // Reconstruct diff lhs entrypoint
  if (last_lhs_word_cursor != NULL) {
    while (last_lhs_word_cursor->prev != NULL) {
      last_lhs_word_cursor = last_lhs_word_cursor->prev;
    }
  }
  diff->lhs.words.first = last_lhs_word_cursor;

  // Rewind the operation stack to the beginning
  if (entry != NULL) {
    while (entry->prev != NULL) {
      entry = entry->prev;
    }
  }

  gplayback_patch operations = {entry};
  return operations;
}
