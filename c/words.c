#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "log.h"
#include "segments.h"
#include "writestr.h"

typedef struct gplayback_word {
  const char *ptr;
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

// test

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

gplayback_word_list_entry *bol(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not find BOL based on NULL pointer");
  while (word->prev != NULL &&
         word->prev->item.line_idx == word->item.line_idx) {
    word = word->prev;
  }
  return word;
}

gplayback_word_list_entry *eol(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not find EOL based on NULL pointer");
  while (word->next != NULL &&
         word->next->item.line_idx == word->item.line_idx) {
    word = word->next;
  }
  return word;
}

gplayback_word_list_entry *lineprev(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not take prev line word from NULL pointer");
  if (word->prev != NULL && word->prev->item.line_idx == word->item.line_idx) {
    word = word->prev;
  }
  return word;
}

gplayback_word_list_entry *linenext(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not take next line word from NULL pointer");
  if (word->next != NULL && word->next->item.line_idx == word->item.line_idx) {
    word = word->next;
  }
  return word;
}

char *debug_word_list(const char *label, gplayback_word_list word_list) {
  char *out = malloc(512);
  size_t offset = 0;
  size_t capacity = 512;

  memset(out, 0, 512);

#define w(...) writestr(out, offset, capacity, __VA_ARGS__)

  gplayback_word_list_entry *word_entry = word_list.first;
  if (word_entry != NULL) {
    do {
      gplayback_word word = word_entry->item;
      if (word.match == NULL) {
        w("[UNMATCHED]");
      }
      w("[%.*s] word: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
        (int)strlen(label), label, word.line_idx, word.col_idx, word.len,
        word.word_id, word.len, word.ptr);
      if (word.match != NULL) {
        w("   -> matched with: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
          word.match->item.line_idx, word.match->item.col_idx,
          word.match->item.len, word.match->item.word_id, word.match->item.len,
          word.match->item.ptr);
      }
    } while ((word_entry = word_entry->next));
  }

  return out;
}

gplayback_word_list_entry *delete_word(gplayback_word_list_entry *word) {
  assert(word != NULL, "Can not delete NULL word");

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
  assert(word != NULL, "Can not delete words until EOL based on NULL pointer");

  int line_idx = word->item.line_idx;

  do {
    word = delete_word(word);
    if (word != NULL && word->item.line_idx != line_idx) {
      break;
    }
  } while (word != NULL);

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

void free_word_list(gplayback_word_list list) {
  gplayback_word_list_entry *entry = list.first;
  while (entry != NULL) {
    gplayback_word_list_entry *next = entry->next;
    free(entry);
    entry = next;
  }
}

int get_last_wordid(gplayback_word_list_entry *first_word) {
  if (first_word == NULL) {
    return 0;
  }
  int word_id = 0;
  gplayback_flags dbg_list = {0};
  do {
    if (dbg_list.count > first_word->item.word_id &&
        dbg_list.items[first_word->item.word_id] == true) {
      dbg_log("Duplicate word id %d found in word list\n",
              first_word->item.word_id);
    }
    da_replace(dbg_list, first_word->item.word_id, true);
    if (first_word->item.word_id > word_id) {
      word_id = first_word->item.word_id;
    }
  } while ((first_word = first_word->next));
  return word_id;
}

gplayback_word_list_entry *insert_word_copy(gplayback_word_list_entry *src,
                                            gplayback_word_list_entry *dest) {
  assert(src != NULL, "Can not insert NULL word");
  assert(dest != NULL, "Can not insert word to NULL pointer");

  gplayback_word_list_entry *src_copy =
      malloc(sizeof(gplayback_word_list_entry));

  *src_copy = *src;
  src_copy->item.word_id =
      get_last_wordid(find_first_word_in_wordlist(dest)) + 1;

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

  return dest;
}

bool is_dirty_line(gplayback_word_list_entry *start) {
  int line_idx = start->item.line_idx;
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

bool is_whitespace(char c) {
  return c == GPLAYBACK_TOKEN_SPACE || c == GPLAYBACK_TOKEN_TAB;
}

bool is_whitespace_or_newline(char c) {
  return is_whitespace(c) || c == GPLAYBACK_TOKEN_NEWLINE;
}

void mark_word_visited(gplayback_flags *visited,
                       gplayback_word_list_entry *word, bool visited_state) {
  da_replace((*visited), word->item.word_id, visited_state);
}

void mark_words_visited_until_eol(gplayback_flags *visited,
                                  gplayback_word_list_entry *word,
                                  bool visited_state) {
  assert(word != NULL, "Can not mark words until EOL from NULL word");
  do {
    mark_word_visited(visited, word, visited_state);
  } while ((word = word->next) != NULL &&
           word->item.line_idx == word->prev->item.line_idx);
}

void move_words_until_eol(gplayback_word_list_entry *src,
                          gplayback_word_list_entry *dest, bool append_mode) {
  assert(src != NULL, "Can not move words from NULL word");
  assert(dest != NULL, "Can not move words to NULL word");

  int line_idx = src->item.line_idx;

  gplayback_word_list_entry_refs refs = {0};

  do {
    da_append(refs, src);
  } while ((src = src->next) && src->item.line_idx == line_idx);

  for (int i = 0; i < refs.count; i++) {
    gplayback_word_list_entry *entry;

    if (append_mode) {
      entry = refs.items[i];
    } else {
      entry = refs.items[refs.count - i - 1];
    }

    gplayback_word_list_entry *next = entry->next;
    gplayback_word_list_entry *prev = entry->prev;

    // Glue next and prev together at current position
    if (next != NULL) {
      next->prev = prev;
    }
    if (prev != NULL) {
      prev->next = next;
    }

    if (append_mode) {
      // Append case
      entry->prev = dest;
      entry->next = dest->next;
      if (dest->next != NULL) {
        dest->next->prev = entry;
      }
      dest->next = entry;
      dest = entry;
    } else {
      // Insert case
      entry->prev = dest->prev;
      if (dest->prev != NULL) {
        dest->prev->next = entry;
      }
      entry->next = dest;
      dest->prev = entry;
      dest = entry;
    }
  }

  free(refs.items);
}

void approach_by_one(int *number, int target) {
  if (*number > target) {
    *number -= 1;
  } else if (target > *number) {
    *number += 1;
  }
}

bool has_next_it(int number, int target, int mod) {
  if (target >= 0) {
    return number < (target + mod);
  }
  return number > (target - mod);
}

gplayback_word_list_entry *iterate_word(gplayback_word_list_entry *word,
                                        int direction) {
  if (direction >= 0) {
    return word->next;
  }
  return word->prev;
}

void move_words_absolute_until_eol(gplayback_word_list_entry *src,
                                   int move_amount) {
  assert(src != NULL, "Moving words from NULL word not allowed");

  bool append_mode = true;
  gplayback_word_list_entry *dest = src;

  int direction;
  if (move_amount > 0) {
    direction = 1;
    append_mode = true;
    move_amount += 1;
  } else {
    direction = -1;
    append_mode = false;
    move_amount -= 1;
  }

  for (int i = 0; has_next_it(i, move_amount, 0);
       approach_by_one(&i, move_amount)) {
    int line_idx = dest->item.line_idx;

    do {
      gplayback_word_list_entry *following = iterate_word(dest, direction);
      if (following == NULL) {
        goto outer;
      } else if (following->item.line_idx != line_idx) {
        break;
      }
      dest = following;
    } while (1);

    if (has_next_it(i, move_amount, -1)) {
      dest = iterate_word(dest, direction);
      assert(dest != NULL, "Attempted to move past boundaries of word list");
    }
  }
outer:

  return move_words_until_eol(src, dest, append_mode);
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

void modify_words_linenum(gplayback_word_list_entry *entry, int limit,
                          int modify_amount) {
  assert(entry != NULL, "Can not modify line number of NULL word");
  int line_idx = entry->item.line_idx;

  /*
   * Bar
   * bie
   * doll
   * xxx
   * yyy
   * zzz
   */

  do {
    if (limit != -1 && entry->item.line_idx != line_idx) {
      if (--limit <= 0) {
        break;
      }
      line_idx = entry->item.line_idx;
    }

    entry->item.line_idx += modify_amount;
  } while ((entry = entry->next));
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
  if (chr == GPLAYBACK_TOKEN_SPACE || chr == GPLAYBACK_TOKEN_TAB ||
      chr == GPLAYBACK_TOKEN_NEWLINE) {
    return GPLAYBACK_TOKEN_SPACE;
  }
  return chr;
}

gplayback_word_list word_list(gplayback_slice text) {
  gplayback_word_list word_list = {0};
  gplayback_word_list_entry *last_entry = NULL;
  int line_idx = 0;
  int col_idx = 0;
  int word_id = 0;
  const char *cursor = NULL;

  char lastchar = GPLAYBACK_TOKEN_NEVER;

  for (size_t i = 0; i < text.len; i++) {
    if (i == 0 || ((!is_whitespace(text.ptr[i]) && is_whitespace(lastchar)) ||
                   lastchar == GPLAYBACK_TOKEN_NEWLINE)) {

      // Commit existing word and begin new one
      if (cursor != NULL) {
        append_wordlist_word(cursor, text.ptr + i - cursor, line_idx, col_idx,
                             word_id++, last_entry, word_list);
      }

      cursor = text.ptr + i;
    }

    if (lastchar == GPLAYBACK_TOKEN_NEWLINE) {
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

int word_subset_of_word(gplayback_word haystack, gplayback_word needle,
                        bool strict) {
  if (needle.len > haystack.len) {
    return -1;
  }

  int match_count = 0;
  for (int i = 0; i < haystack.len; i++) {
    char needle_char = strict ? needle.ptr[match_count]
                              : word_matchchr(needle.ptr[match_count]);

    char haystack_char =
        strict ? haystack.ptr[i] : word_matchchr(haystack.ptr[i]);

    if (needle_char == haystack_char) {
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
