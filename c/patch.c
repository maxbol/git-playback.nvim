#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "constants.h"
#include "diff.c"
#include "log.h"

typedef struct {
  int line;
  int column;
  /*size_t chr;*/
} gplayback_cursorpos;

typedef struct {
  int type;
  void *data;
  gplayback_cursorpos cursor;
  gplayback_cursorpos cursor_after;
} gplayback_vm_operation;

typedef struct gplayback_vm_operation_entry {
  gplayback_vm_operation item;
  struct gplayback_vm_operation_entry *next;
  struct gplayback_vm_operation_entry *prev;
} gplayback_vm_operation_entry;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_word_after;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_row_after;

typedef struct {
  size_t char_len;
} gplayback_vm_op_delete_words;

typedef struct {
  size_t no_of_lines;
} gplayback_vm_op_delete_rows;

typedef struct {
  size_t no_of_lines;
  int move_amount;
} gplayback_vm_op_move_rows;

typedef struct {
  gplayback_vm_operation_entry *first;
} gplayback_patch;

typedef struct {
  int anchor_line;
  int lines_amount;
} deleteset;

typedef struct {
  gplayback_word_list_entry *anchor;
  int lines_amount;
  int move_amount;
} moveset;

gplayback_vm_operation_entry *
append_operation_entry(gplayback_vm_operation_entry *entry, int type,
                       void *data, gplayback_cursorpos cursor,
                       gplayback_cursorpos cursor_after) {
  // Split lines
  gplayback_vm_operation op = {
      type,
      data,
      cursor,
      cursor_after,
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
                                cursor, cursor);
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
      assert(line_anchor->next != NULL, "Moveset line anchor has no next word "
                                        "entry, this should not happen");
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

  return append_operation_entry(entry, GPLAYBACK_OP_MOVE_ROWS, op_data, cursor,
                                cursor);
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

void debug_patch(gplayback_patch patch) {
  printf("Operations generated:\n");
  gplayback_vm_operation_entry *entry = patch.first;
  while (entry != NULL) {
    switch (entry->item.type) {
    case GPLAYBACK_OP_INSERT_WORD_AFTER: {
      gplayback_vm_op_insert_word_after *data = entry->item.data;
      printf(" >> Insert word after [src=\"%.*s\"]\n", (int)data->src.len,
             data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      printf(" >> Insert row after [src=\"%.*s\"]\n", (int)data->src.len,
             data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_MOVE_ROWS: {
      gplayback_vm_op_move_rows *data = entry->item.data;
      printf(" >> Move rows [no_of_lines=%zu, move_amount=%d]\n",
             data->no_of_lines, data->move_amount);
      break;
    }
    case GPLAYBACK_OP_DELETE_ROWS: {
      gplayback_vm_op_delete_rows *data = entry->item.data;
      printf(" >> Delete rows [len=%zu]\n", data->no_of_lines);
      break;
    }
    case GPLAYBACK_OP_DELETE_WORDS: {
      gplayback_vm_op_delete_words *data = entry->item.data;
      printf(" >> Delete words [len=%zu]\n", data->char_len);
      break;
    }
    case GPLAYBACK_OP_CONCAT_ROWS: {
      printf(" >> Concat rows\n");
      break;
    }
    case GPLAYBACK_OP_SPLIT_ROWS: {
      printf(" >> Split rows\n");
      break;
    }
    }
    printf(" + Cursor: %d:%d -> %d:%d\n", entry->item.cursor.line,
           entry->item.cursor.column, entry->item.cursor_after.line,
           entry->item.cursor_after.column);

    printf("\n");

    entry = entry->next;
  }
}

gplayback_patch generate_patch(gplayback_diff diff) {
  gplayback_vm_operation_entry *entry = NULL;

  moveset ml = {0, 0, 0};
  deleteset dl = {-1, 0};

  gplayback_cursorpos cursor = {0, 0};

  gplayback_word_list_entry *lhs_anchor = NULL;
  gplayback_word_list_entry *rhs_anchor = NULL;
  gplayback_word_list_entry *lhs_word_cursor = diff.lhs.words.first;
  gplayback_word_list_entry *rhs_word_cursor = diff.rhs.words.first;

  gplayback_flags lhs_visited = {0};
  gplayback_flags lhs_inserts_processed = {0};
  int next_wordid = get_next_wordid(lhs_word_cursor);
  da_capacity(lhs_visited, next_wordid);
  da_capacity(lhs_inserts_processed, next_wordid);

  bool line_is_dirty = false;

  do {
    dbg_log("lhs_word_cursor: %d, %.*s\n", lhs_word_cursor->item.word_id,
            (int)lhs_word_cursor->item.len, lhs_word_cursor->item.ptr);
    if (lhs_word_cursor == NULL) {
      break;
    }

    bool is_newline = false;

    if (lhs_word_cursor->prev == NULL ||
        lhs_word_cursor->item.line_idx !=
            lhs_word_cursor->prev->item.line_idx) {
      /*line = &diff.lhs.lines.items[lhs_word_cursor->item.line_idx];*/
      cursor.line = lhs_word_cursor->item.line_idx;
      is_newline = true;
      lhs_anchor = NULL;
      rhs_anchor = NULL;

      line_is_dirty = is_dirty_line(lhs_word_cursor, cursor.line);
    }

    cursor.column = lhs_word_cursor->item.col_idx;

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
    do {
      if (rhs_word_cursor == NULL) {
        break;
      }

      if (rhs_word_cursor->item.match != NULL) {
        if (!is_word_visited(lhs_inserts_processed,
                             *rhs_word_cursor->item.match)) {
          // If the corresponding word on the LHS hasn't been visited
          // by the patch generator, then it is too early to insert any more
          // words from the RHS
          dbg_log("RHS word match is not visited: %.*s\n",
                  (int)rhs_word_cursor->item.len, rhs_word_cursor->item.ptr);
          break;
        }
        dbg_log("RHS word is matched: %.*s\n", (int)rhs_word_cursor->item.len,
                rhs_word_cursor->item.ptr);
        continue;
      }

      gplayback_line rhs_line =
          diff.rhs.lines.items[rhs_word_cursor->item.line_idx];

      if (!rhs_line.dirty) {
        // Insert entire line

        gplayback_vm_op_insert_row_after *op_data =
            malloc(sizeof(gplayback_vm_op_insert_row_after));

        op_data->src = subslice(rhs_line.words->item.ptr, 0, rhs_line.char_len);

        gplayback_cursorpos il_cursor = {0};

        il_cursor.line = lhs_word_cursor->item.line_idx;
        il_cursor.column = lhs_word_cursor->item.col_idx;

        dbg_log("Adding INSERT_ROW_AFTER op, cursorpos: %d, %d\n",
                il_cursor.line, il_cursor.column);

        entry = append_operation_entry(entry, GPLAYBACK_OP_INSERT_ROW_AFTER,
                                       op_data, il_cursor, il_cursor);

        modify_words_linenum(lhs_word_cursor, -1, 1);

        lhs_word_cursor =
            insert_words_copy_until_eol(lhs_word_cursor, rhs_word_cursor);

        mark_words_visited_until_eol(&lhs_visited, lhs_word_cursor, true);
        mark_words_visited_until_eol(&lhs_inserts_processed, lhs_word_cursor,
                                     true);

        // Fast-forward rhs cursor to next line
        do {
          if (rhs_word_cursor->item.line_idx != rhs_line.line_num) {
            break;
          }
        } while ((rhs_word_cursor = rhs_word_cursor->next));
      } else {
        // Insert word

        gplayback_vm_op_insert_word_after *op_data =
            malloc(sizeof(gplayback_vm_op_insert_word_after));

        op_data->src =
            subslice(rhs_word_cursor->item.ptr, 0, rhs_word_cursor->item.len);

        gplayback_cursorpos iw_cursor = {0};
        if (lhs_word_cursor->prev != NULL) {
          iw_cursor.line = lhs_word_cursor->prev->item.line_idx;
          iw_cursor.column = lhs_word_cursor->prev->item.col_idx +
                             lhs_word_cursor->prev->item.len;
        }

        dbg_log("Adding INSERT_WORD_AFTER op, cursorpos: %d, %d\n",
                iw_cursor.line, iw_cursor.column);

        entry = append_operation_entry(entry, GPLAYBACK_OP_INSERT_WORD_AFTER,
                                       op_data, iw_cursor, iw_cursor);

        modify_words_colnum_until_eol(lhs_word_cursor,
                                      rhs_word_cursor->item.len);

        lhs_word_cursor = insert_word_copy(rhs_word_cursor, lhs_word_cursor);

        mark_word_visited(&lhs_visited, lhs_word_cursor, true);
        mark_word_visited(&lhs_inserts_processed, lhs_word_cursor, true);
      }
    } while ((rhs_word_cursor = rhs_word_cursor->next));

    // Add DELETE_WORD, CONCAT_ROWS and SPLIT_ROWS operations as needed
    // But first check if visited (necessary because the word might have have
    // been moved here from elsewhere)
    if (!is_word_visited(lhs_visited, *lhs_word_cursor)) {
      if (lhs_word_cursor->item.match == NULL) {
        // Remove word
        gplayback_vm_op_delete_words *op_data =
            malloc(sizeof(gplayback_vm_op_delete_words));

        op_data->char_len = lhs_word_cursor->item.len;

        dbg_log("Adding DELETE_WORDS op, cursorpos: %d,%d\n", cursor.line,
                cursor.column);

        entry = append_operation_entry(entry, GPLAYBACK_OP_DELETE_WORDS,
                                       op_data, cursor, cursor);

        // This should not be necessary, commenting out...
        /*mark_word_visited(&lhs_visited, lhs_word_cursor, true);*/

        lhs_word_cursor = delete_word(lhs_word_cursor);

        if (lhs_word_cursor == NULL) {
          break;
        }

        modify_words_colnum_until_eol(lhs_word_cursor, op_data->char_len);

        // We can't catch up to this position on the rhs, so let's just
        // skip to the next lhs word cursor position
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

          if (lhs_lastchar == GPLAYBACK_TOKEN_NEWLINE &&
              rhs_lastchar != GPLAYBACK_TOKEN_NEWLINE) {
            // Concat lines
            entry = append_operation_entry(entry, GPLAYBACK_OP_CONCAT_ROWS,
                                           NULL, cursor, cursor);
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
                                           cursor, cursor);
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

    // End of line, see if line should be moved
    if (!is_word_visited(lhs_visited, *lhs_word_cursor) &&
        (lhs_word_cursor->next == NULL ||
         lhs_word_cursor->next->item.line_idx !=
             lhs_word_cursor->item.line_idx)) {
      if (lhs_anchor != NULL && rhs_anchor != NULL &&
          lhs_anchor->item.line_idx != rhs_anchor->item.line_idx) {
        int target_line = 0;

        int move_amount = calc_move_amount(rhs_anchor, lhs_word_cursor);

        if (move_amount != 0) {
          if (ml.lines_amount > 0) {
            entry = append_ml_operation_entry(entry, lhs_anchor, &ml,
                                              &lhs_inserts_processed);

            // We have to redo the calculation here, because the positions might
            move_amount = calc_move_amount(rhs_anchor, lhs_word_cursor);
            // have shifted around with the move
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

  // Rewind the operation stack to the beginning
  while (entry->prev != NULL) {
    entry = entry->prev;
  }

  gplayback_patch operations = {entry};
  return operations;
}
