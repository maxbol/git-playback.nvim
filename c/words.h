#ifndef WORDS_H
#define WORDS_H
#include <stdbool.h>
#include <stdlib.h>

#include "segments.h"

typedef struct gplayback_word {
  char *ptr;
  struct gplayback_word_list_entry *match;
  int line_idx;
  int col_idx;
  int len;
  int word_id;
} gplayback_word;

typedef struct gplayback_word_list_entry {
  gplayback_word item;
  struct gplayback_word_list_entry *next;
  struct gplayback_word_list_entry *prev;
} gplayback_word_list_entry;

typedef struct {
  gplayback_word_list_entry **items;
  size_t count;
  size_t capacity;
} gplayback_word_list_entry_refs;

typedef struct {
  gplayback_word_list_entry *first;
} gplayback_word_list;

typedef struct {
  bool *items;
  size_t count;
  size_t capacity;
} gplayback_flags;

#define append_wordlist_word(v_ptr, v_len, v_line_idx, v_col_idx, v_word_id,   \
                             v_last_entry, v_word_list)                        \
  do {                                                                         \
    gplayback_word_list_entry *entry =                                         \
        malloc(sizeof(gplayback_word_list_entry));                             \
                                                                               \
    entry->item.ptr = v_ptr;                                                   \
    entry->item.len = v_len;                                                   \
    entry->item.line_idx = v_line_idx;                                         \
    entry->item.col_idx = v_col_idx - entry->item.len;                         \
    entry->item.match = NULL;                                                  \
    entry->item.word_id = v_word_id;                                           \
                                                                               \
    if (v_last_entry != NULL) {                                                \
      v_last_entry->next = entry;                                              \
      entry->prev = v_last_entry;                                              \
    } else {                                                                   \
      entry->prev = NULL;                                                      \
      v_word_list.first = entry;                                               \
    }                                                                          \
    v_last_entry = entry;                                                      \
  } while (0)

gplayback_word_list_entry *bol(gplayback_word_list_entry *word);
gplayback_word_list_entry *eol(gplayback_word_list_entry *word);
gplayback_word_list_entry *lineprev(gplayback_word_list_entry *word);
gplayback_word_list_entry *linenext(gplayback_word_list_entry *word);
char *debug_word_list(const char *label, gplayback_word_list word_list);
gplayback_word_list_entry *delete_word(gplayback_word_list_entry *word);
gplayback_word_list_entry *
delete_words_until_eol(gplayback_word_list_entry *word);
gplayback_word_list_entry *
find_first_word_in_wordlist(gplayback_word_list_entry *word);
void free_word_list(gplayback_word_list list);
int get_last_wordid(gplayback_word_list_entry *first_word);
gplayback_word_list_entry *insert_word_copy(gplayback_word_list_entry *src,
                                            gplayback_word_list_entry *dest);
gplayback_word_list_entry *
insert_words_copy_until_eol(gplayback_word_list_entry *src,
                            gplayback_word_list_entry *dest);
bool is_dirty_line(gplayback_word_list_entry *word);
bool is_word_visited(gplayback_flags visited, gplayback_word_list_entry word);
bool is_whitespace(char c);
bool is_whitespace_or_newline(char c);
void mark_word_visited(gplayback_flags *visited,
                       gplayback_word_list_entry *word, bool visited_state);
void mark_words_visited_until_eol(gplayback_flags *visited,
                                  gplayback_word_list_entry *word,
                                  bool visited_state);
void move_words_until_eol(gplayback_word_list_entry *src,
                          gplayback_word_list_entry *dest, bool append_mode);
void move_words_absolute_until_eol(gplayback_word_list_entry *src,
                                   int move_amount);
void modify_words_colnum_until_eol(gplayback_word_list_entry *word, int amount);
void modify_words_linenum(gplayback_word_list_entry *entry, int limit,
                          int modify_amount);
void modify_words_linenum_backwards(gplayback_word_list_entry *start, int limit,
                                    int modify_amount);
void modify_words_linenum_until_eol(gplayback_word_list_entry *word,
                                    int modify_amount);
gplayback_word_list word_list(gplayback_slice text);
int word_subset_of_word(gplayback_word a, gplayback_word b, bool strict);

#endif // !WORDS_H
