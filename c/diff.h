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
  /*gplayback_lines lines;*/
  gplayback_slice slice;
} gplayback_text;

typedef struct {
  unsigned int moveword_min_word_amount;
  float moveline_entropy_treshold;
} gplayback_generate_diff_opts;

typedef struct {
  unsigned int lhs_anchor;
  unsigned int lines_amount;
} gplayback_diff_moveline;

typedef struct {
  unsigned int lhs_start;
  unsigned int words_amount;
} gplayback_diff_movewords;

typedef struct {
  gplayback_text lhs;
  gplayback_text rhs;
  gplayback_diff_moveline movelines[WORD_MAX_MOVELINES];
  gplayback_diff_movewords movewords[WORD_MAX_MOVEWORDS];
} gplayback_diff;

typedef struct {
  unsigned int from;
  unsigned int to;
  unsigned int target;
} gplayback_diff_word_process_span;

gplayback_text diff_clone_text(gplayback_text text);
gplayback_diff diff_clone(gplayback_diff *diff);
char *diff_debug(gplayback_diff *diff);
void diff_free(gplayback_diff *diff);
void diff_match_words(gplayback_text *outer, gplayback_text *inner);
gplayback_diff diff_generate(gplayback_slice lhs, gplayback_slice rhs,
                             gplayback_generate_diff_opts opts);

#endif // !DIFF_H
