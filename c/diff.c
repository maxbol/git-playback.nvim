#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "diff.h"
#include "log.h"
#include "segments.h"
#include "words.h"
#include "writestr.h"

#define w(...) writestr(out, offset, capacity, __VA_ARGS__);

char *debug_lines(const char *label, gplayback_lines lines) {
  char *out = malloc(512);
  size_t offset = 0;
  size_t capacity = 512;

  memset(out, 0, 512);

  w("%.*s lines:\n", (int)strlen(label), label);
  for (int i = 0; i < lines.count; i++) {
    gplayback_line line = lines.items[i];
    w("%.*s %zu: ", (int)strlen(label), label, line.line_num);
    if (line.match != NULL) {
      w("[EXACT] ");
    } else if (line.dirty) {
      w("[CONTAINS-MATCHES] ");
    } else {
      w("[UNMATCHED] ");
    }
    w("%.*s", (int)line.char_len, line.words->item.ptr);
    w("\nWords: ");
    gplayback_word_list_entry *e = line.words;
    int word_idx = 0;
    do {
      w("%d", e->item.word_id);
      if (e->next != NULL && word_idx + 1 < line.word_len) {
        w(", ");
      }
    } while ((e = e->next) && (++word_idx) < line.word_len);
    w("\n");
  }

  return out;
}

char *debug_diff(gplayback_diff diff) {
  gplayback_word_list lhs_word_list = diff.lhs.words;
  gplayback_word_list rhs_word_list = diff.rhs.words;

  char *out = malloc(512);
  size_t offset = 0;
  size_t capacity = 512;

  memset(out, 0, 512);

  w("LHS lines:\n");
  char *lhs_lines_debug = debug_lines("LHS", diff.lhs.lines);
  w("%.*s", (int)strlen(lhs_lines_debug), lhs_lines_debug);
  free(lhs_lines_debug);

  w("RHS lines:\n");
  char *rhs_lines_debug = debug_lines("RHS", diff.rhs.lines);
  w("%.*s", (int)strlen(rhs_lines_debug), rhs_lines_debug);
  free(rhs_lines_debug);

  w("LHS word list:\n");
  char *lhs_wl_debug = debug_word_list("LHS", lhs_word_list);
  w("%.*s", (int)strlen(lhs_wl_debug), lhs_wl_debug);
  free(lhs_wl_debug);

  w("RHS word list:\n");
  char *rhs_wl_debug = debug_word_list("RHS", rhs_word_list);
  w("%.*s", (int)strlen(rhs_wl_debug), rhs_wl_debug);
  free(rhs_wl_debug);

  return out;
}

void free_diff_lines(gplayback_lines lines) { da_free(lines); }

void free_diff(gplayback_diff diff) {
  free_word_list(diff.lhs.words);
  free_word_list(diff.rhs.words);
  free_diff_lines(diff.lhs.lines);
  free_diff_lines(diff.rhs.lines);
  free_slice_buf(diff.lhs.slice);
  free_slice_buf(diff.rhs.slice);
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

bool lines_identical(gplayback_line *a, gplayback_line *b) {
  if (a->char_len == 0 || b->char_len == 0 || a->char_len != b->char_len ||
      a->word_len != b->word_len) {
    return false;
  }
  assert(a->words != NULL && b->words != NULL, "Line words are NULL");
  const char *a_ptr = a->words->item.ptr;
  const char *b_ptr = b->words->item.ptr;
  for (int i = 0; i < a->char_len; i++) {
    if (a_ptr[i] != b_ptr[i]) {
      return false;
    }
  }
  return true;
}

void match_identical_lines(gplayback_text *outer, gplayback_text *inner) {
  gplayback_lines *outer_lines = &outer->lines;
  gplayback_lines *inner_lines = &inner->lines;

  for (int i = 0; i < outer_lines->count; i++) {
    gplayback_line *outer_line = &outer_lines->items[i];

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
    }
  }
}

void match_lines(gplayback_text *outer, gplayback_text *inner) {
  gplayback_lines *outer_lines = &outer->lines;
  gplayback_lines *inner_lines = &inner->lines;

  int word_id = get_last_wordid(inner->words.first);

  // Primacy is given to identically matched lines
  match_identical_lines(outer, inner);

  gplayback_word_list_entry *outer_word_cursor = outer->words.first;

  do {
    if (outer_word_cursor->item.match != NULL) {
      continue;
    }

    gplayback_line *outer_line =
        &outer_lines->items[outer_word_cursor->item.line_idx];

    if (outer_line->match != NULL) {
      continue;
    }

    gplayback_word_list_entry *inner_word_cursor = inner->words.first;

    // Prevent over-matching of words only containing whitespaces or
    // newlines
    bool outer_strict =
        is_whitespace_or_newline(outer_word_cursor->item.ptr[0]);

    do {
      if (inner_word_cursor->item.match != NULL) {
        continue;
      }

      gplayback_line *inner_line =
          &inner_lines->items[inner_word_cursor->item.line_idx];

      dbg_log("Testing outer vs inner: %d vs %d\n",
              outer_word_cursor->item.word_id, inner_word_cursor->item.word_id);

      bool strict = outer_strict ||
                    is_whitespace_or_newline(inner_word_cursor->item.ptr[0]);

      int match_idx = word_subset_of_word(inner_word_cursor->item,
                                          outer_word_cursor->item, strict);

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
                               NULL,
                               inner_word_cursor->item.line_idx,
                               inner_word_cursor->item.col_idx + outer_end,
                               inner_word_cursor->item.len - outer_end,
                               ++word_id};
        gplayback_word_list_entry *e =
            malloc(sizeof(gplayback_word_list_entry));
        e->item = word;
        e->next = new_entry;
        new_entry = e;
      }

      gplayback_word word = {inner_word_cursor->item.ptr + match_idx,
                             outer_word_cursor,
                             inner_word_cursor->item.line_idx,
                             inner_word_cursor->item.col_idx + match_idx,
                             outer_word_cursor->item.len,
                             0};

      if (match_idx > 0) {
        word.word_id = ++word_id;

        gplayback_word_list_entry *e =
            malloc(sizeof(gplayback_word_list_entry));
        e->item = word;
        e->next = new_entry;
        new_entry = e;

        inner_word_cursor->item.match = NULL;
        inner_word_cursor->item.len = match_idx;
        outer_word_cursor->item.match = new_entry;
      } else {
        word.word_id = inner_word_cursor->item.word_id;

        inner_word_cursor->item = word;
        outer_word_cursor->item.match = inner_word_cursor;
      }

      inner_word_cursor->next = new_entry;
    } while ((inner_word_cursor = inner_word_cursor->next));
  } while ((outer_word_cursor = outer_word_cursor->next));
}

gplayback_diff generate_diff(gplayback_slice lhs, gplayback_slice rhs) {
  gplayback_diff diff = {0};

  diff.lhs.slice = copy_slice(lhs);
  diff.rhs.slice = copy_slice(rhs);

  diff.lhs.words = word_list(diff.lhs.slice);
  diff.rhs.words = word_list(diff.rhs.slice);

  diff.lhs.lines = lines(diff.lhs.words);
  diff.rhs.lines = lines(diff.rhs.words);

  dbg_log("Matching outer vs inner: lhs vs rhs\n");
  match_lines(&diff.lhs, &diff.rhs);
  dbg_log("Matching outer vs inner: rhs vs lhs\n");
  match_lines(&diff.rhs, &diff.lhs);

  return diff;
}
