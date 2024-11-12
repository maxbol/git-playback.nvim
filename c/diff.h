#ifndef DIFF_H
#define DIFF_H

#include <stdbool.h>

#include "words.h"

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

int debug_diff(gplayback_diff diff, char out[], size_t out_len);
void free_diff(gplayback_diff *diff);
gplayback_lines lines(gplayback_word_list word_list);
bool lines_identical(gplayback_line *a, gplayback_line *b);
void match_lines(gplayback_text *outer, gplayback_text *inner);
gplayback_diff generate_diff(gplayback_slice lhs, gplayback_slice rhs);

#endif // !DIFF_H
