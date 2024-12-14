#ifndef WORDS_H
#define WORDS_H
#include <stdbool.h>
#include <stdlib.h>

#include "slice.h"

#ifndef WORD_LIST_LEN
#define WORD_LIST_LEN 8192
#endif

#ifndef DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN
#define DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN 512
#endif

#ifndef WORD_LINE_MAX_LEN
#define WORD_LINE_MAX_LEN 8192
#endif

#ifndef WORD_MAX_LINES
#define WORD_MAX_LINES 8192
#endif

#ifndef WORD_MAX_MOVEWORDS
#define WORD_MAX_MOVEWORDS 4096
#endif

#ifndef WORD_MAX_MOVELINES
#define WORD_MAX_MOVELINES 512
#endif

typedef struct gplayback_word {
  unsigned int match;
  unsigned int line_idx;
  unsigned int col_idx;
  unsigned int len;
  unsigned int word_id;
  const char *ptr;
} gplayback_word;

typedef struct gplayback_word_list_entry {
  unsigned int next;
  unsigned int prev;
  gplayback_word item;
} gplayback_word_list_entry;

typedef struct {
  gplayback_word_list_entry **items;
  size_t count;
  size_t capacity;
} gplayback_word_list_entry_refs;

typedef struct {
  unsigned int head;
  gplayback_word_list_entry items[WORD_LIST_LEN];
} gplayback_word_list_entries;

typedef struct {
  // Entrypoint into linked list of words. Pointers point into entries.
  unsigned int first;
  // Append-only cache of entries, appended in order they get added, order never
  // changed.
  gplayback_word_list_entries entries;
} gplayback_word_list;

gplayback_word_list_entry words_get_entry(gplayback_word_list *word_list,
                                          unsigned int word_id);
gplayback_word_list_entry *
words_get_entry_pointer(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_create_entry(gplayback_word_list *word_list,
                                gplayback_word word);
unsigned int words_append(gplayback_word_list *word_list, gplayback_word word,
                          gplayback_word_list_entry *last_word_entry);
unsigned int words_insert(gplayback_word_list *word_list, gplayback_word word,
                          unsigned int before_word_id);
unsigned int words_copy_line(gplayback_word_list *src_list,
                             gplayback_word_list *dest_list,
                             unsigned int src_start, unsigned int dest_before);
unsigned int words_delete(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_delete_words_until_eol(gplayback_word_list *word_list,
                                          unsigned int word_id);
unsigned int words_first(gplayback_word_list *word_list);
unsigned int words_last(gplayback_word_list *word_list);
void words_recalc_first(gplayback_word_list *word_list);
unsigned int words_bol(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_eol(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_prev(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_next(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_prevl(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_nextl(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_prevlt(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_nextlt(gplayback_word_list *word_list, unsigned int word_id);
unsigned int words_line_charlen(gplayback_word_list *word_list,
                                unsigned int word_id);
unsigned int words_line_wordlen(gplayback_word_list *word_list,
                                unsigned int word_id);
char *words_allocprint_word_list(const char *label,
                                 gplayback_word_list *word_list,
                                 gplayback_word_list *match_list);
bool words_line_has_matches(gplayback_word_list *word_list,
                            unsigned int word_id);
bool words_char_is_whitespace(char c);
bool words_char_is_whitespace_or_newline(char c);
void words_move_line(gplayback_word_list *word_list, unsigned int word_id,
                     unsigned int dest_before, bool append_mode);
void words_move_line_absolute(gplayback_word_list *word_list_src,
                              unsigned int src_start, int move_amount);
void words_move(gplayback_word_list *word_list, unsigned int word_id,
                unsigned int dest_before, bool append_mode);
void words_modify_colnum_until_eol(gplayback_word_list *word_list,
                                   unsigned int word_id, int amount);
void words_modify_linenums(gplayback_word_list *word_list, unsigned int word_id,
                           unsigned int no_of_lines, int modify_amount);
void words_modify_linenums_backwards(gplayback_word_list *word_list,
                                     unsigned int word_id,
                                     unsigned int no_of_lines,
                                     int modify_amount);
gplayback_word_list words_create_list(gplayback_slice text);
int words_find_in_word(gplayback_word haystack, gplayback_word needle,
                       bool strict);
void words_transfer_slice(gplayback_word_list *word_list,
                          gplayback_slice from_slice, gplayback_slice to_slice);
unsigned int words_find_anchors(gplayback_word_list *word_list,
                                unsigned int *out);
unsigned int words_find_line(gplayback_word_list *word_list,
                             unsigned int line_idx);
void words_recalc_line_numbers(gplayback_word_list *word_list);
void words_recalc_col_numbers(gplayback_word_list *word_list,
                              unsigned int word_id);
bool words_is_linesep(gplayback_word word);
unsigned int words_print_wordlist(char *out, unsigned int out_len,
                                  gplayback_word_list *word_list);
unsigned int words_print_line(char *out, unsigned int out_len,
                              gplayback_word_list *word_list,
                              unsigned int word_id);
unsigned int words_print_line_with_highlights(char *out, unsigned int out_len,
                                              gplayback_word_list *word_list,
                                              unsigned int *word_ids,
                                              unsigned int word_ids_len,
                                              const char *normal_color,
                                              const char *highlight_color);
unsigned int words_print_word(char *out, unsigned int out_len,
                              gplayback_word_list *word_list,
                              unsigned int word_id);
unsigned int words_span_count(gplayback_word_list *word_list,
                              unsigned int first, unsigned int last);

#endif // !WORDS_H
