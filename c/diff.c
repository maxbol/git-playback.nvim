#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "words.c"

typedef struct gplayback_line {
  bool dirty;
  size_t word_len;
  size_t char_len;
  size_t line_num;
  gplayback_word_list_entry *words;
  struct gplayback_line *match;
} gplayback_line;

typedef struct {
  gplayback_line *items;
  size_t count;
  size_t capacity;
} gplayback_lines;

typedef struct {
  gplayback_word_list words;
  gplayback_lines lines;
} gplayback_text;

typedef struct {
  gplayback_text lhs;
  gplayback_text rhs;
} gplayback_diff;

void debug_diff(gplayback_diff diff) {
  gplayback_word_list lhs_word_list = diff.lhs.words;
  gplayback_word_list rhs_word_list = diff.rhs.words;
  gplayback_lines lhs_lines = diff.lhs.lines;
  gplayback_lines rhs_lines = diff.rhs.lines;

  gplayback_word_list_entry *w_e = lhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("lhs_word: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
           word.line_idx, word.col_idx, word.len, word.word_id, word.len,
           word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.col_idx,
             word.match->item.len, word.match->item.word_id,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));

  w_e = rhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("rhs_word: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
           word.line_idx, word.col_idx, word.len, word.word_id, word.len,
           word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.col_idx,
             word.match->item.len, word.match->item.word_id,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));
}

void free_diff_lines(gplayback_lines *lines) { da_free(lines); }

void free_diff(gplayback_diff *diff) {
  free_diff_word_list(diff->lhs.words);
  free_diff_word_list(diff->rhs.words);
  free_diff_lines(&diff->lhs.lines);
  free_diff_lines(&diff->rhs.lines);
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
  if (a->words == NULL || b->words == NULL) {
    exit(1);
  }
  const char *a_ptr = a->words->item.ptr;
  const char *b_ptr = b->words->item.ptr;
  for (int i = 0; i < a->char_len; i++) {
    if (a_ptr[i] != b_ptr[i]) {
      return false;
    }
  }
  return true;
}

void match_lines(gplayback_text *outer, gplayback_text *inner) {
  gplayback_lines *outer_lines = &outer->lines;
  gplayback_lines *inner_lines = &inner->lines;

  int word_id = get_next_wordid(inner->words.first);

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
                                   NULL,
                                   inner_word_cursor->item.line_idx,
                                   inner_word_cursor->item.col_idx + outer_end,
                                   inner_word_cursor->item.len - outer_end,
                                   word_id++};
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
            word.word_id = word_id++;

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
        } while ((inner_word_cursor = inner_word_cursor->next) &&
                 (++inner_word_idx) < inner_line->word_len);
      } while ((outer_word_cursor = outer_word_cursor->next) != NULL &&
               (++outer_word_idx) < outer_line->word_len);
    }
  }
}

gplayback_diff generate_diff(gplayback_slice lhs, gplayback_slice rhs) {
  gplayback_diff diff = {0};

  diff.lhs.words = word_list(lhs);
  diff.rhs.words = word_list(rhs);

  diff.lhs.lines = lines(diff.lhs.words);
  diff.rhs.lines = lines(diff.rhs.words);

  match_lines(&diff.lhs, &diff.rhs);
  match_lines(&diff.rhs, &diff.lhs);

  return diff;
}
