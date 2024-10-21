#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "log.h"
#include "motions.h"

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

void debug_diff(gplayback_diff diff) {
  gplayback_word_list lhs_word_list = diff.lhs.words;
  gplayback_word_list rhs_word_list = diff.rhs.words;
  gplayback_lines lhs_lines = diff.lhs.lines;
  gplayback_lines rhs_lines = diff.rhs.lines;

  gplayback_word_list_entry *w_e = lhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("lhs_word: (line %d, len %d) \"%.*s\"\n", word.line_idx, word.len,
           word.len, word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, len %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.len,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));

  w_e = rhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("rhs_word: (line %d, len %d) \"%.*s\"\n", word.line_idx, word.len,
           word.len, word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, len %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.len,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));

  for (int i = 0; i < lhs_lines.count; i++) {
    gplayback_line line = lhs_lines.items[i];
    printf("lhs_line: %d (word len %zu, char len %zu)\n", i, line.word_len,
           line.char_len);
    if (line.match != NULL) {
      printf("  - matched with rhs_line %zu\n", line.match->line_num);
    }
    gplayback_word_list_entry *word_cursor = line.words;
    int word_idx = 0;
    do {
      gplayback_word lhs_word = word_cursor->item;
      printf("  - lhs_word: %.*s\n", (int)lhs_word.len, lhs_word.ptr);
    } while ((word_cursor = word_cursor->next) && (++word_idx) < line.word_len);
  }

  for (int i = 0; i < rhs_lines.count; i++) {
    gplayback_line line = rhs_lines.items[i];
    printf("rhs_line: %d (word len %zu, char len %zu)\n", i, line.word_len,
           line.char_len);
    if (line.match != NULL) {
      printf("  - matched with lhs_line %zu\n", line.match->line_num);
    }
    gplayback_word_list_entry *word_cursor = line.words;
    int word_idx = 0;
    do {
      gplayback_word rhs_word = word_cursor->item;
      printf("  - rhs_word: %.*s\n", (int)rhs_word.len, rhs_word.ptr);
    } while ((word_cursor = word_cursor->next) && (++word_idx) < line.word_len);
  }
}

char word_matchchr(char chr) {
  if (chr == ' ' || chr == '\t' || chr == '\n') {
    return ' ';
  }
  return chr;
}

int word_subset_of_word(gplayback_word haystack, gplayback_word needle) {
  if (needle.len > haystack.len) {
    return -1;
  }

  int match_count = 0;
  for (int i = 0; i < haystack.len; i++) {
    if (word_matchchr(needle.ptr[match_count]) ==
        word_matchchr(haystack.ptr[i])) {
      match_count++;
    } else {
      match_count = 0;
    }
    if (match_count == needle.len) {
      return i - needle.len + 1;
    }
    if (needle.len - match_count > haystack.len - i) {
      return -1;
    }
  }

  return -1;
}

bool lines_identical(gplayback_line *a, gplayback_line *b) {
  if (a->char_len == 0 || b->char_len == 0 || a->char_len != b->char_len ||
      a->word_len != b->word_len) {
    return false;
  }
  if (a->words == NULL || b->words == NULL) {
    exit(1);
  }
  char *a_ptr = a->words->item.ptr;
  char *b_ptr = b->words->item.ptr;
  for (int i = 0; i < a->char_len; i++) {
    if (a_ptr[i] != b_ptr[i]) {
      return false;
    }
  }
  return true;
}

void match_lines(gplayback_lines *outer_lines, gplayback_lines *inner_lines) {
  for (int i = 0; i < outer_lines->count; i++) {
    gplayback_line *outer_line = &outer_lines->items[i];

    if (outer_line->match != NULL) {
      continue;
    }

    for (int j = 0; j < inner_lines->count; j++) {
      gplayback_line *inner_line = &inner_lines->items[j];

      if (inner_line->match != NULL) {
        continue;
      }

      if (!outer_line->dirty && !inner_line->dirty &&
          lines_identical(outer_line, inner_line)) {
        gplayback_word_list_entry *outer_word_cursor = outer_line->words;
        gplayback_word_list_entry *inner_word_cursor = inner_line->words;

        int outer_word_idx = 0;
        int inner_word_idx = 0;

        do {
          outer_word_cursor->item.match = inner_word_cursor;
          inner_word_cursor->item.match = outer_word_cursor;
        } while ((inner_word_cursor = inner_word_cursor->next) &&
                 (outer_word_cursor = outer_word_cursor->next) &&
                 (++outer_word_idx) < outer_line->word_len &&
                 (++inner_word_idx) < inner_line->word_len);

        outer_line->dirty = true;
        inner_line->dirty = true;

        outer_line->match = inner_line;
        inner_line->match = outer_line;

        break;
      }

      gplayback_word_list_entry *outer_word_cursor = outer_line->words;
      int outer_word_idx = 0;

      do {
        if (outer_word_cursor->item.match != NULL) {
          continue;
        }

        int inner_word_idx = 0;
        gplayback_word_list_entry *inner_word_cursor = inner_line->words;

        do {
          if (inner_word_cursor->item.match != NULL) {
            continue;
          }

          int match_idx = word_subset_of_word(inner_word_cursor->item,
                                              outer_word_cursor->item);

          if (match_idx < 0) {
            continue;
          }

          outer_line->dirty = true;
          inner_line->dirty = true;

          if (match_idx == 0 &&
              outer_word_cursor->item.len == inner_word_cursor->item.len) {
            outer_word_cursor->item.match = inner_word_cursor;
            inner_word_cursor->item.match = outer_word_cursor;
            break;
          }

          gplayback_word_list_entry *new_entry = inner_word_cursor->next;

          int outer_end = outer_word_cursor->item.len + match_idx;
          if (outer_end < inner_word_cursor->item.len) {
            gplayback_word word = {inner_word_cursor->item.ptr + outer_end,
                                   NULL, inner_word_cursor->item.line_idx,
                                   inner_word_cursor->item.len - outer_end};
            gplayback_word_list_entry *e =
                malloc(sizeof(gplayback_word_list_entry));
            e->item = word;
            e->next = new_entry;
            new_entry = e;
          }

          gplayback_word word = {
              inner_word_cursor->item.ptr + match_idx, outer_word_cursor,
              inner_word_cursor->item.line_idx, outer_word_cursor->item.len};

          if (match_idx > 0) {
            gplayback_word_list_entry *e =
                malloc(sizeof(gplayback_word_list_entry));
            e->item = word;
            e->next = new_entry;
            new_entry = e;

            inner_word_cursor->item.match = NULL;
            inner_word_cursor->item.len = match_idx;
            outer_word_cursor->item.match = new_entry;
          } else {
            inner_word_cursor->item = word;
            outer_word_cursor->item.match = inner_word_cursor;
          }

          inner_word_cursor->next = new_entry;
        } while ((inner_word_cursor = inner_word_cursor->next) &&
                 (++inner_word_idx) < inner_line->word_len);
      } while ((outer_word_cursor = outer_word_cursor->next) != NULL &&
               (++outer_word_idx) < outer_line->word_len);
    }
  }
}

gplayback_lines lines(gplayback_word_list word_list) {
  gplayback_lines lines = {0};
  gplayback_word_list_entry *entry = word_list.first;

  if (entry == NULL) {
    return lines;
  }

  int word_len = 0;
  int char_len = 0;
  int line_idx = 0;
  gplayback_word_list_entry *word_cursor = entry;
  do {
    if (entry->item.line_idx != line_idx) {
      gplayback_line line = {0};
      line.words = word_cursor;
      line.word_len = word_len;
      line.char_len = char_len;
      line.line_num = line_idx;
      line.dirty = false;
      line.match = NULL;
      da_append(lines, line);

      word_len = 1;
      char_len = entry->item.len;
      line_idx = entry->item.line_idx;
      word_cursor = entry;
    } else {
      word_len++;
      char_len += entry->item.len;
    }
  } while ((entry = entry->next) != NULL);

  gplayback_line line = {0};
  line.words = word_cursor;
  line.word_len = word_len;
  line.char_len = char_len;
  line.line_num = line_idx;
  line.dirty = false;
  line.match = NULL;
  da_append(lines, line);

  return lines;
}

bool is_whitespace(char c) { return c == ' ' || c == '\t'; }

gplayback_word_list word_list(gplayback_slice text) {
  gplayback_word_list word_list = {0};
  gplayback_word_list_entry *last_entry = NULL;
  int line_idx = 0;
  char *cursor = NULL;

  char lastchar = '\xff';
  for (size_t i = 0; i < text.len; i++) {
    bool lastchar_is_newline = text.ptr[i - 1] == '\n';
    if (i > 0 &&
        ((!is_whitespace(text.ptr[i]) && is_whitespace(text.ptr[i - 1])) ||
         lastchar_is_newline)) {
      // Commit existing word and begin new one
      if (cursor != NULL) {
        gplayback_word_list_entry *entry =
            malloc(sizeof(gplayback_word_list_entry));

        entry->item.ptr = cursor;
        entry->item.line_idx = line_idx;
        entry->item.match = NULL;
        entry->item.len = text.ptr + i - cursor;

        if (last_entry != NULL) {
          last_entry->next = entry;
        } else {
          word_list.first = entry;
        }
        last_entry = entry;
      }

      cursor = text.ptr + i;
    } else if (i == 0) {
      cursor = text.ptr + i;
    }

    if (lastchar_is_newline) {
      line_idx++;
    }
  }

  if (cursor != NULL) {
    gplayback_word_list_entry *entry =
        malloc(sizeof(gplayback_word_list_entry));

    entry->item.ptr = cursor;
    entry->item.line_idx = line_idx;
    entry->item.match = NULL;
    entry->item.len = text.ptr + text.len - cursor;

    if (last_entry != NULL) {
      last_entry->next = entry;
    } else {
      word_list.first = entry;
    }
    last_entry = entry;
  }
  return word_list;
}

void free_diff_word_list(gplayback_word_list list) {
  gplayback_word_list_entry *entry = list.first;
  while (entry != NULL) {
    gplayback_word_list_entry *next = entry->next;
    free(entry);
    entry = next;
  }
}

void free_diff_lines(gplayback_lines *lines) { da_free(lines); }

void free_diff(gplayback_diff *diff) {
  free_diff_word_list(diff->lhs.words);
  free_diff_word_list(diff->rhs.words);
  free_diff_lines(&diff->lhs.lines);
  free_diff_lines(&diff->rhs.lines);
}

struct delete_lines {
  int lines_amount;
};
gplayback_vm_operation_entry *
delete_lines_op(gplayback_vm_operation_entry *entry,
                gplayback_cursorpos *cursor, struct delete_lines *dl) {
  if (dl->lines_amount == 0) {
    return entry;
  }

  gplayback_vm_op_delete_rows *op_data =
      malloc(sizeof(gplayback_vm_op_delete_rows));

  op_data->no_of_lines = dl->lines_amount;

  dbg_log("Decreasing cursor.line by %d\n", dl->lines_amount);
  cursor->line -= dl->lines_amount;

  dbg_log("Adding DELETE_ROWS op: prev - %p; cursor - %zu,%zu\n", entry,
          cursor->line, cursor->column);

  dl->lines_amount = 0;

  return append_operation_entry(entry, GPLAYBACK_OP_DELETE_ROWS, op_data,
                                *cursor, *cursor);
}

struct move_lines {
  int lines_amount;
  int move_amount;
};
gplayback_vm_operation_entry *moves_line_op(gplayback_vm_operation_entry *entry,
                                            gplayback_cursorpos *cursor,
                                            struct move_lines *ml) {
  if (ml->lines_amount == 0) {
    return entry;
  }

  gplayback_vm_op_move_rows *op_data =
      malloc(sizeof(gplayback_vm_op_move_rows));

  op_data->no_of_lines = ml->lines_amount;
  op_data->move_amount = ml->move_amount;

  cursor->line -= ml->lines_amount;

  ml->lines_amount = 0;
  ml->move_amount = 0;

  dbg_log(
      "Adding MOVE_ROWS op - prev: %p, cursorpos: %zu,%zu, moveamount: %d\n",
      entry, cursor->line, cursor->column, ml->move_amount);

  return append_operation_entry(entry, GPLAYBACK_OP_MOVE_ROWS, op_data, *cursor,
                                *cursor);
}

gplayback_patch generate_patch(gplayback_diff diff) {
  gplayback_vm_operation_entry *entry = NULL;

  struct move_lines ml = {0, 0};
  struct delete_lines dl = {0};

  gplayback_word_list_entry *rhs_word_cursor = diff.rhs.words.first;
  gplayback_cursorpos cursor = {0, 0};

  int line_adjust = 0;
  for (int i = 0; i < diff.lhs.lines.count; i++) {
    gplayback_line line = diff.lhs.lines.items[i];

    cursor.column = 0;
    dbg_log("cursor line: %zu\n", cursor.line);

    if (line.dirty == true) {
      if (dl.lines_amount > 0) {
        entry = delete_lines_op(entry, &cursor, &dl);
      }

      dbg_log("dirty line #%zu: %.*s\n", line.line_num, (int)line.char_len,
              line.words->item.ptr);

      /*gplayback_word_list_entry *lhs_line_anchor = line.words->prev;*/
      /*if (lhs_line_anchor != NULL) {*/
      /*  gplayback_word_list_entry *rhs_line_anchor =*/
      /*      lhs_line_anchor->item.match;*/
      /*}*/

      // TODO(2024-10-20, Max Bolotin): Straighten this spaghetti
      gplayback_line *matched_line = line.match;
      if (matched_line != NULL) {
        dbg_log("matched line #%zu: %.*s\n", matched_line->line_num,
                (int)matched_line->char_len, matched_line->words->item.ptr);
        if (matched_line->line_num != cursor.line) {
          int move_amount = matched_line->line_num - line.line_num;
          if (move_amount != ml.move_amount) {
            if (ml.lines_amount > 0) {
              dbg_log("[xxx] Adding %d line move op\n", ml.lines_amount);
              entry = moves_line_op(entry, &cursor, &ml);
            }
            ml.move_amount = move_amount;
          }
          ml.lines_amount++;
          dbg_log("move_amount=%d, lines_amount=%d\n", ml.move_amount,
                  ml.lines_amount);
        } else if (ml.lines_amount > 0) {
          dbg_log("[yyy] Adding %d line move op\n", ml.lines_amount);
          entry = moves_line_op(entry, &cursor, &ml);
        }

        cursor.line++;
        continue;
      }

      if (ml.lines_amount > 0) {
        dbg_log("[zzz] Adding %d line move op\n", ml.lines_amount);
        entry = moves_line_op(entry, &cursor, &ml);
      }

      gplayback_word_list_entry *lhs_word_cursor = line.words;
      int lhs_word_idx = 0;
      do {
        if (lhs_word_cursor->item.match == NULL) {

          // Remove word
          gplayback_vm_op_delete_words *op_data =
              malloc(sizeof(gplayback_vm_op_delete_words));

          op_data->char_len = lhs_word_cursor->item.len;

          dbg_log("Adding DELETE_WORDS op, cursorpos: %zu,%zu\n", cursor.line,
                  cursor.column);

          entry = append_operation_entry(entry, GPLAYBACK_OP_DELETE_ROWS,
                                         op_data, cursor, cursor);

          // We can't catch up to this position on the rhs, so let's just
          // skip to the next lhs cursor position
          continue;
        } else {
          char lhs_lastchar =
              lhs_word_cursor->item.ptr[lhs_word_cursor->item.len - 1];
          char rhs_lastchar =
              rhs_word_cursor->item.ptr[rhs_word_cursor->item.len - 1];

          if (lhs_lastchar == '\n' && rhs_lastchar != '\n') {
            // Concat lines
            entry = append_operation_entry(entry, GPLAYBACK_OP_CONCAT_ROWS,
                                           NULL, cursor, cursor);
          } else if (lhs_lastchar != '\n' && rhs_lastchar == '\n') {
            // Split lines
            entry = append_operation_entry(entry, GPLAYBACK_OP_SPLIT_ROWS, NULL,
                                           cursor, cursor);
          }
          cursor.column += lhs_word_cursor->item.len;
        }

        // Have the rhs cursor catch up to our position, inserting
        // words/lines as we go
        do {
          if (rhs_word_cursor == NULL) {
            break;
          }

          if (rhs_word_cursor->item.match != NULL) {
            // TODO(2024-10-21, Max Bolotin): This seems to be adding a level of
            // lookahead on both sides of the equation... maybe just
            // rwc->item.match == lwc and then skip assigning rwc->next to rwc?
            // Look into it.
            if (rhs_word_cursor->item.match == lhs_word_cursor->next) {
              rhs_word_cursor = rhs_word_cursor->next;
              break;
            }
            continue;
          }

          gplayback_line rhs_line =
              diff.rhs.lines.items[rhs_word_cursor->item.line_idx];

          if (!rhs_line.dirty) {
            // Insert entire line

            gplayback_vm_op_insert_row_after *op_data =
                malloc(sizeof(gplayback_vm_op_insert_row_after));

            op_data->src =
                subslice(rhs_line.words->item.ptr, 0, rhs_line.char_len);

            gplayback_cursorpos il_cursor = cursor;

            cursor.line++;
            cursor.column = 0;

            gplayback_cursorpos il_cursor_after = cursor;

            dbg_log("Adding INSERT_ROW_AFTER op, cursorpos: %zu, %zu\n",
                    il_cursor.line, il_cursor.column);

            gplayback_vm_operation op = {
                GPLAYBACK_OP_INSERT_ROW_AFTER,
                (void *)op_data,
                il_cursor,
                il_cursor_after,
            };

            gplayback_vm_operation_entry *next_entry =
                malloc(sizeof(gplayback_vm_operation_entry));

            next_entry->item = op;
            next_entry->next = NULL;
            next_entry->prev = entry;

            if (entry != NULL) {
              entry->next = next_entry;
            }

            entry = next_entry;

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

            op_data->src = subslice(rhs_word_cursor->item.ptr, 0,
                                    rhs_word_cursor->item.len);

            gplayback_cursorpos iw_cursor = cursor;

            cursor.column += rhs_word_cursor->item.len;

            gplayback_cursorpos iw_cursor_after = cursor;

            dbg_log("Adding INSERT_WORD_AFTER op, cursorpos: %zu, %zu\n",
                    iw_cursor.line, iw_cursor.column);

            gplayback_vm_operation op = {
                GPLAYBACK_OP_INSERT_WORD_AFTER,
                (void *)op_data,
                iw_cursor,
                iw_cursor_after,
            };

            gplayback_vm_operation_entry *next_entry =
                malloc(sizeof(gplayback_vm_operation_entry));

            next_entry->item = op;
            next_entry->next = NULL;
            next_entry->prev = entry;

            if (entry != NULL) {
              entry->next = next_entry;
            }

            entry = next_entry;
          }
        } while ((rhs_word_cursor = rhs_word_cursor->next));

        dbg_log("-|\n");
      } while ((++lhs_word_idx) < line.word_len &&
               (lhs_word_cursor = lhs_word_cursor->next));

    } else {
      if (ml.lines_amount > 0) {
        dbg_log("[aaa] Adding %d line move op\n", ml.lines_amount);
        entry = moves_line_op(entry, &cursor, &ml);
      }

      dl.lines_amount++;
    }

    dbg_log("Increasing cursor.line by 1\n");
    cursor.line++;
  }

  if (ml.lines_amount > 0) {
    dbg_log("Adding remaining move ops\n");
    entry = moves_line_op(entry, &cursor, &ml);
  }

  if (dl.lines_amount > 0) {
    dbg_log("Adding remaining delete lines ops\n");
    entry = delete_lines_op(entry, &cursor, &dl);
  }

  // Rewind the operation stack to the beginning
  while (entry->prev != NULL) {
    entry = entry->prev;
  }

  gplayback_patch operations = {entry};
  return operations;
}

gplayback_diff generate_diff(gplayback_slice lhs, gplayback_slice rhs) {
  gplayback_diff diff = {0};

  diff.lhs.words = word_list(lhs);
  diff.rhs.words = word_list(rhs);

  diff.lhs.lines = lines(diff.lhs.words);
  diff.rhs.lines = lines(diff.rhs.words);

  match_lines(&diff.lhs.lines, &diff.rhs.lines);
  match_lines(&diff.rhs.lines, &diff.lhs.lines);

  printf("After matching:\n");
  debug_diff(diff);

  return diff;
}
