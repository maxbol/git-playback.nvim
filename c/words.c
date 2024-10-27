#include <stdbool.h>
#include <stdio.h>

#include "arrays.h"
#include "assert.c"
#include "log.h"
#include "segments.c"

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

gplayback_word_list_entry *delete_word(gplayback_word_list_entry *word) {
  if (word == NULL) {
    dbg_log("Warning: Trying to delete NULL word, this has no effect\n");
    return word;
  }
  gplayback_word_list_entry *prev = word->prev;
  gplayback_word_list_entry *next = word->next;

  if (prev != NULL) {
    prev->next = next;
  }

  if (next != NULL) {
    next->prev = prev;
  }

  free(word);

  return next;
}

gplayback_word_list_entry *
delete_words_until_eol(gplayback_word_list_entry *word) {
  if (word == NULL) {
    dbg_log("Warning: Trying to delete words until EOL from NULL anchor word, "
            "this has no effect\n");
    return word;
  }
  int line_idx = word->item.line_idx;
  while (word->next != NULL) {
    word = delete_word(word);
    if (word->item.line_idx != line_idx) {
      break;
    }
  }
  return word;
}

gplayback_word_list_entry *
find_first_word_in_wordlist(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not find word based on NULL pointer");
  while (word->prev != NULL) {
    word = word->prev;
  }
  return word;
}

void free_diff_word_list(gplayback_word_list list) {
  gplayback_word_list_entry *entry = list.first;
  while (entry != NULL) {
    gplayback_word_list_entry *next = entry->next;
    free(entry);
    entry = next;
  }
}

int get_next_wordid(gplayback_word_list_entry *first_word) {
  if (first_word == NULL) {
    return 0;
  }
  int word_id = first_word->item.word_id;
  while (first_word->next != NULL) {
    first_word = first_word->next;
    if (first_word->item.word_id > word_id) {
      word_id = first_word->item.word_id;
    }
  }
  return word_id;
}

gplayback_word_list_entry *insert_word_copy(gplayback_word_list_entry *src,
                                            gplayback_word_list_entry *dest) {
  if (src == NULL || dest == NULL) {
    dbg_log("Warning: Trying to insert NULL word, this has no effect\n");
    return dest;
  }
  gplayback_word_list_entry *src_copy =
      malloc(sizeof(gplayback_word_list_entry));

  *src_copy = *src;
  src_copy->item.word_id = get_next_wordid(find_first_word_in_wordlist(dest));

  gplayback_word_list_entry *prev = dest->prev;

  if (prev != NULL) {
    prev->next = src_copy;
  }

  src_copy->prev = prev;
  src_copy->next = dest;
  dest->prev = src_copy;

  return src_copy;
}

gplayback_word_list_entry *
insert_words_copy_until_eol(gplayback_word_list_entry *src,
                            gplayback_word_list_entry *dest) {
  if (src == NULL || dest == NULL) {
    dbg_log("Warning: Trying to insert words until EOL from NULL word, "
            "this has no effect\n");
    return dest;
  }

  int line_idx = src->item.line_idx;

  do {
    dest = insert_word_copy(src, dest);
    assert(dest != NULL, "Inserting word failed");
    dest = dest->next;
  } while ((src = src->next) && src->item.line_idx == line_idx);

  return src;
}

bool is_dirty_line(gplayback_word_list_entry *start, int line_idx) {
  do {
    if (start->item.match != NULL) {
      return true;
    }
  } while ((start = start->next) != NULL && start->item.line_idx == line_idx);
  return false;
}

bool is_word_visited(gplayback_flags visited, gplayback_word_list_entry word) {
  return visited.items[word.item.word_id];
}

bool is_whitespace(char c) { return c == ' ' || c == '\t'; }

void mark_word_visited(gplayback_flags *visited,
                       gplayback_word_list_entry *word) {
  da_replace(visited, word->item.word_id, true);
}

void mark_words_visited_until_eol(gplayback_flags *visited,
                                  gplayback_word_list_entry *word) {
  if (word == NULL) {
    dbg_log("Warning: Trying to mark words until EOL from NULL word, "
            "this has no effect\n");
    return;
  }
  do {
    mark_word_visited(visited, word);
  } while ((word = word->next) != NULL &&
           word->item.line_idx == word->prev->item.line_idx);
}

void move_words_until_eol(gplayback_word_list_entry *src,
                          gplayback_word_list_entry *dest, bool append_mode) {
  assert(src != NULL, "Can not move words from NULL word");
  assert(dest != NULL, "Can not move words to NULL word");

  int line_idx = src->item.line_idx;

  do {
    gplayback_word_list_entry *next = src->next;
    gplayback_word_list_entry *prev = src->prev;

    // Glue next and prev together at current position
    if (next != NULL) {
      next->prev = prev;
    }
    if (prev != NULL) {
      prev->next = next;
    }

    if (append_mode) {
      // Append case
      src->prev = dest;
      src->next = dest->next;
    } else {
      // Insert case
      src->prev = dest->prev;
      src->next = dest;
    }

  } while ((src = src->next) && src->item.line_idx == line_idx);
}

void move_words_absolute_until_eol(gplayback_word_list_entry *src,
                                   int move_amount) {
  if (src == NULL) {
    dbg_log("Warning: Trying to move words until EOL from NULL word, "
            "this has no effect\n");
    return;
  }

  gplayback_word_list_entry *dest = src;
  for (int i = 0; i < move_amount; i++) {
    while (dest->next != NULL &&
           dest->next->item.line_idx == dest->item.line_idx) {
      dest = dest->next;
    }
  }

  assert(dest != NULL, "No more lines available to move");

  return move_words_until_eol(src, dest, true);
}

void modify_words_colnum_until_eol(gplayback_word_list_entry *word,
                                   int amount) {
  if (word == NULL) {
    dbg_log("Warning: Trying to modify column number of NULL word, this has no "
            "effect\n");
    return;
  }
  do {
    word->item.col_idx += amount;
  } while (word->next != NULL &&
           word->item.line_idx == word->next->item.line_idx &&
           (word = word->next));
}

void modify_words_linenum(gplayback_word_list_entry *start, int limit,
                          int modify_amount) {
  assert(start != NULL, "Can not modify line number of NULL word");
  int line_idx = start->item.line_idx;

  do {
    if (limit != -1 && start->item.line_idx != line_idx) {
      if (--limit <= 0) {
        break;
      }
      line_idx = start->item.line_idx;
    }

    start->item.line_idx += modify_amount;
  } while ((start = start->next));
}

void modify_words_linenum_backwards(gplayback_word_list_entry *start, int limit,
                                    int modify_amount) {
  if (start == NULL) {
    dbg_log("Warning: Trying to modify line number of NULL word, this has no "
            "effect\n");
    return;
  }
  while (limit == -1 || (limit--) > 0) {
    start->item.line_idx += modify_amount;

    if (start->prev == NULL) {
      break;
    }

    start = start->prev;
  }
}

void modify_words_linenum_until_eol(gplayback_word_list_entry *word,
                                    int amount) {
  if (word == NULL) {
    dbg_log("Warning: Trying to modify line number of NULL word, this has no "
            "effect\n");
    return;
  }
  int line_idx;
  do {
    line_idx = word->item.line_idx;
    word->item.line_idx += amount;
  } while (word->next != NULL && line_idx == word->next->item.line_idx &&
           (word = word->next));
}

char word_matchchr(char chr) {
  if (chr == ' ' || chr == '\t' || chr == '\n') {
    return ' ';
  }
  return chr;
}

gplayback_word_list word_list(gplayback_slice text) {
  gplayback_word_list word_list = {0};
  gplayback_word_list_entry *last_entry = NULL;
  int line_idx = 0;
  int col_idx = 0;
  int word_id = 0;
  char *cursor = NULL;

  char lastchar = '\xff';
  for (size_t i = 0; i < text.len; i++) {
    if (i > 0 && ((!is_whitespace(text.ptr[i]) && is_whitespace(lastchar)) ||
                  lastchar == '\n')) {

      // Commit existing word and begin new one
      if (cursor != NULL) {
        append_wordlist_word(cursor, text.ptr + i - cursor, line_idx, col_idx,
                             word_id++, last_entry, word_list);
      }

      cursor = text.ptr + i;
    } else if (i == 0) {
      cursor = text.ptr + i;
    }

    if (lastchar == '\n') {
      line_idx++;
      col_idx = 1;
    } else {
      col_idx++;
    }

    lastchar = text.ptr[i];
  }

  if (cursor != NULL) {
    append_wordlist_word(cursor, text.ptr + text.len - cursor, line_idx,
                         col_idx, word_id, last_entry, word_list);
  }

  return word_list;
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
