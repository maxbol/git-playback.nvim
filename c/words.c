#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "escapestr.h"
#include "log.h"
#include "slice.h"
#include "words.h"
#include "writestr.h"

gplayback_word_list_entry words_get_entry(gplayback_word_list *word_list,
                                          gplayback_word_id word_id) {
  assert(word_id != 0, "Trying to get word with zero ID, this is not allowed");
  assert(word_id < word_list->entries.head,
         "Trying to get word with ID out of bounds");
  gplayback_word_list_entry entry = word_list->entries.items[word_id];
  assert(entry.item.word_id != 0,
         "Word at ID %d seems to be uninstantiated, this will lead to "
         "undefined behavior",
         entry.item.word_id);
  return entry;
}

gplayback_word_list_entry *
words_get_entry_pointer(gplayback_word_list *word_list,
                        gplayback_word_id word_id) {
  assert(word_id != 0,
         "Trying to get word pointer with zero ID, this is not allowed");
  assert(word_id < word_list->entries.head,
         "Trying to get word with ID out of bounds");
  gplayback_word_list_entry *entry = &word_list->entries.items[word_id];
  assert(entry->item.word_id != 0,
         "Word at ID %d seems to be uninstantiated, this will lead to "
         "undefined behavior",
         entry->item.word_id);
  return entry;
}

gplayback_word_id words_create_entry(gplayback_word_list *word_list,
                                     gplayback_word word) {
  if (word_list->entries.head >= WORD_LIST_LEN) {
    error("Word list entry limit reached");
  }
  word.word_id = word_list->entries.head++;
  word_list->entries.items[word.word_id] = (gplayback_word_list_entry){
      .item = word,
  };

  return word.word_id;
}

/*
 * Appends a word to the end of the word list. If last_word_entry is NULL, the
 * word is appended at the end of the list, otherwise it is appended after the
 * word designated by last_word_entry. However, this is presumed to be the last
 * word of the list, so no care is taken to change the backwards references of
 * subsequent words. `last_word_entry` should only be used in cases where the
 * last entry is already known, and only for performance reasons. Returns the ID
 * of the newly created word.
 */
gplayback_word_id words_append(gplayback_word_list *word_list,
                               gplayback_word word,
                               gplayback_word_list_entry *last_word_entry) {
  gplayback_word_id new_word_id = words_create_entry(word_list, word);

  if (word_list->first != 0) {
    if (last_word_entry == NULL) {
      gplayback_word_id last_word_id = words_last(word_list);
      assert(last_word_id != 0, "Last word ID can't be zero");
      last_word_entry = words_get_entry_pointer(word_list, last_word_id);
    }

    gplayback_word_list_entry *new_word_entry =
        words_get_entry_pointer(word_list, new_word_id);

    new_word_entry->prev = last_word_entry->item.word_id;
    last_word_entry->next = new_word_id;
  } else {
    word_list->first = new_word_id;
  }

  return new_word_id;
}

/*
 * Inserts a word before the word designated by next_word_id. If next_word_id is
 * zero, the word is inserted at the beginning of the list. Returns the ID of
 * the newly created word.
 *
 * Note: Unline `words_append`, this function is safe to use for inserting a
 * word anywhere in the list.
 */
gplayback_word_id words_insert(gplayback_word_list *word_list,
                               gplayback_word word,
                               gplayback_word_id next_word_id) {
  gplayback_word_id new_word_id = words_create_entry(word_list, word);

  gplayback_word_list_entry *new_word =
      words_get_entry_pointer(word_list, new_word_id);

  gplayback_word_list_entry *next_word =
      words_get_entry_pointer(word_list, next_word_id);

  if (next_word->prev != 0) {
    gplayback_word_list_entry *prev_word =
        words_get_entry_pointer(word_list, next_word->prev);

    prev_word->next = new_word_id;
    new_word->prev = next_word->prev;
  } else {
    word_list->first = new_word_id;
  }

  new_word->next = next_word_id;
  next_word->prev = new_word_id;

  return new_word_id;
}

gplayback_word_id words_copy_line(gplayback_word_list *src_list,
                                  gplayback_word_list *dest_list,
                                  gplayback_word_id src_start,
                                  gplayback_word_id dest_before) {
  gplayback_word_id cursor = words_bol(src_list, src_start);
  gplayback_word_id dest_word_id = dest_before;

  while (cursor != 0) {
    gplayback_word_list_entry src_word = words_get_entry(src_list, cursor);
    gplayback_word word = src_word.item;
    words_insert(dest_list, word, dest_word_id);
    cursor = words_nextl(src_list, cursor);
  }

  return dest_word_id;
}

gplayback_word_id words_delete(gplayback_word_list *word_list,
                               gplayback_word_id word_id) {
  gplayback_word_list_entry *entry =
      words_get_entry_pointer(word_list, word_id);

  if (entry->prev != 0) {
    gplayback_word_list_entry *prev =
        words_get_entry_pointer(word_list, entry->prev);
    prev->next = entry->next;
  } else {
    word_list->first = entry->next;
  }

  if (entry->next != 0) {
    gplayback_word_list_entry *next =
        words_get_entry_pointer(word_list, entry->next);
    next->prev = entry->prev;
  }

  gplayback_word_id next = entry->next;

  *entry = (gplayback_word_list_entry){0};

  return next;
}

gplayback_word_id words_delete_words_until_eol(gplayback_word_list *word_list,
                                               gplayback_word_id word_id) {
  unsigned int line_idx = words_get_entry(word_list, word_id).item.line_idx;

  while (word_id != 0) {
    gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
    if (entry.item.line_idx != line_idx) {
      break;
    }
    word_id = words_delete(word_list, word_id);
  }

  return word_id;
}

gplayback_word_id words_find_line(gplayback_word_list *word_list,
                                  unsigned int line_idx) {
  gplayback_word_id cursor = word_list->first;
  while (cursor != 0) {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    if (entry.item.line_idx == line_idx) {
      return cursor;
    }
    cursor = entry.next;
  }
  return 0;
}

/*
 * Returns the ID of the next word in the word list if it exists, else returns 0
 */
gplayback_word_id words_next(gplayback_word_list *word_list,
                             gplayback_word_id word_id) {
  gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
  return entry.next;
}

/*
 * Returns the ID of the previous word in the word list if it exists, else
 * returns 0
 */
gplayback_word_id words_prev(gplayback_word_list *word_list,
                             gplayback_word_id word_id) {
  gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
  return entry.prev;
}

/*
 * Returns the ID of the next word in the word list if it exists and is one
 * the same line, else returns 0
 */
gplayback_word_id words_nextl(gplayback_word_list *word_list,
                              gplayback_word_id word_id) {
  gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
  if (entry.next != 0) {
    gplayback_word_list_entry next = words_get_entry(word_list, entry.next);
    if (next.item.line_idx == entry.item.line_idx) {
      return next.item.word_id;
    }
  }
  return 0;
}

/*
 * Returns the ID of the previous word in the word list if it exists and is one
 * the same line, else returns 0
 */
gplayback_word_id words_prevl(gplayback_word_list *word_list,
                              gplayback_word_id word_id) {
  gplayback_word_list_entry word = words_get_entry(word_list, word_id);
  if (word.prev != 0) {
    gplayback_word_list_entry prev = words_get_entry(word_list, word.prev);
    if (prev.item.line_idx == word.item.line_idx) {
      return prev.item.word_id;
    }
  }

  return 0;
}

/*
 * Returns the ID of the next word in the word list if it exists and is one
 * the same line, else returns the current word ID
 */
gplayback_word_id words_nextlt(gplayback_word_list *word_list,
                               gplayback_word_id word_id) {
  gplayback_word_id next_word_id = words_nextl(word_list, word_id);
  if (next_word_id != 0) {
    return next_word_id;
  }
  return word_id;
}

/*
 * Returns the ID of the previous word in the word list if it exists and is one
 * the same line, else returns the current word ID
 */
gplayback_word_id words_prevlt(gplayback_word_list *word_list,
                               gplayback_word_id word_id) {
  gplayback_word_id prev_word_id = words_prevl(word_list, word_id);
  if (prev_word_id != 0) {
    return prev_word_id;
  }
  return word_id;
}

gplayback_word_id words_last(gplayback_word_list *word_list) {
  gplayback_word_id word_id = word_list->first;
  gplayback_word_id next = word_id;
  while ((next = words_next(word_list, next)) != 0) {
    word_id = next;
  }
  return word_id;
}

gplayback_word_id words_bol(gplayback_word_list *word_list,
                            gplayback_word_id word_id) {
  assert(word_id != 0, "Can not find BOL based on zero word");
  gplayback_word_id cursor = word_id;
  while (cursor != 0) {
    word_id = cursor;
    cursor = words_prevl(word_list, cursor);
  }
  return word_id;
}

gplayback_word_id words_eol(gplayback_word_list *word_list,
                            gplayback_word_id word_id) {
  assert(word_id != 0, "Can not find EOL based on zero word");
  gplayback_word_id cursor = word_id;
  while (cursor != 0) {
    word_id = cursor;
    cursor = words_nextl(word_list, cursor);
  }
  return word_id;
}

/*
 * Returns the number of characters on the same line as word
 */
unsigned int words_line_charlen(gplayback_word_list *word_list,
                                gplayback_word_id word_id) {
  word_id = words_bol(word_list, word_id);
  unsigned int char_len = 0;
  do {
    gplayback_word_list_entry word = words_get_entry(word_list, word_id);
    char_len += word.item.len;
  } while ((word_id = words_nextl(word_list, word_id)) != 0);
  return char_len;
}

/*
 * Returns the number of words on the same line as word
 */
unsigned int words_line_wordlen(gplayback_word_list *word_list,
                                gplayback_word_id word_id) {
  word_id = words_bol(word_list, word_id);
  int word_len = 0;
  do {
    word_len++;
  } while ((word_id = words_nextl(word_list, word_id)) != 0);
  return word_len;
}

/*
 * Allocates a string containing a debug representation of the word list
 */
char *words_allocprint_word_list(const char *label,
                                 gplayback_word_list *word_list,
                                 gplayback_word_list *match_list) {
  gplayback_writestr_state ws = writestr_create(512);

  gplayback_word_id cursor = word_list->first;

  if (cursor == 0) {
    return ws.out;
  }

  do {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    gplayback_word word = entry.item;

    if (word.match == 0) {
      writestr(ws, "[UNMATCHED]");
    }

    char escaped_word_buf[DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN] = {0};
    int escaped_len =
        escape_fmt(escaped_word_buf, DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN, "%.*s",
                   (int)word.len, word.ptr);

    writestr(ws, "[%.*s] word: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
             (int)strlen(label), label, word.line_idx, word.col_idx, word.len,
             word.word_id, escaped_len, escaped_word_buf);

    if (word.match != 0) {
      gplayback_word_list_entry match_word =
          words_get_entry(match_list, word.match);
      char escaped_word_buf[DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN] = {0};
      int escaped_len =
          escape_fmt(escaped_word_buf, DEBUG_WORD_LIST_ESCAPEDCHARBUFLEN,
                     "%.*s", (int)match_word.item.len, match_word.item.ptr);
      writestr(
          ws, "   -> matched with: (line %d, col %d, len %d, id %d) \"%.*s\"\n",
          match_word.item.line_idx, match_word.item.col_idx,
          match_word.item.len, match_word.item.word_id, escaped_len,
          escaped_word_buf);
    }
  } while ((cursor = words_next(word_list, cursor)) != 0);

  return ws.out;
}

bool words_line_has_matches(gplayback_word_list *word_list,
                            gplayback_word_id word_id) {
  gplayback_word_id cursor = word_id;
  do {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    if (entry.item.match != 0) {
      return true;
    }
  } while ((cursor = words_nextl(word_list, cursor)) != 0);
  return false;
}

bool words_char_is_whitespace(char c) {
  return c == GPLAYBACK_TOKEN_SPACE || c == GPLAYBACK_TOKEN_TAB;
}

bool words_char_is_whitespace_or_newline(char c) {
  return words_char_is_whitespace(c) || c == GPLAYBACK_TOKEN_NEWLINE;
}

bool words_char_is_newline(char c) { return c == GPLAYBACK_TOKEN_NEWLINE; }

bool words_is_linesep(gplayback_word word) {
  return word.len == 1 && words_char_is_newline(word.ptr[0]);
}

bool words_is_whitespace(gplayback_word word) {
  for (int i = 0; i < word.len; i++) {
    if (!words_char_is_whitespace(word.ptr[i])) {
      return false;
    }
  }
  return true;
}

/*
 * Moves a single word to a new position in the word list. If append_mode is
 * true, the word is appended after the word designated by dest_before,
 * otherwise it is inserted before it. Does not automatically trigger a
 * recalculation of line numbers, so always call words_recalc_line_numbers()
 * after running this function.
 */
void words_move(gplayback_word_list *word_list, gplayback_word_id word_id,
                gplayback_word_id dest_before, bool append_mode) {
  gplayback_word_list_entry *entry =
      words_get_entry_pointer(word_list, word_id);
  gplayback_word_list_entry *dest =
      words_get_entry_pointer(word_list, dest_before);

  assert(word_id != dest_before, "Can't move word to itself");

  if (entry->prev != 0) {
    gplayback_word_list_entry *prev =
        words_get_entry_pointer(word_list, entry->prev);
    prev->next = entry->next;
  } else {
    word_list->first = entry->next;
  }

  if (entry->next != 0) {
    gplayback_word_list_entry *next =
        words_get_entry_pointer(word_list, entry->next);
    next->prev = entry->prev;
  }

  if (append_mode) {
    entry->prev = dest_before;
    entry->next = dest->next;
    if (dest->next != 0) {
      gplayback_word_list_entry *next =
          words_get_entry_pointer(word_list, dest->next);
      next->prev = word_id;
    }
    dest->next = word_id;
  } else {
    entry->prev = dest->prev;
    if (dest->prev != 0) {
      gplayback_word_list_entry *prev =
          words_get_entry_pointer(word_list, dest->prev);
      prev->next = word_id;
    }
    entry->next = dest_before;
    dest->prev = word_id;
  }
}

/*
 * Moves the line containing word id to a position designated by dest_before.
 * If append_mode is ture, the line is appended after the word designated by
 * dest_before, otherwise it is inserted before it. Does not automatically
 * trigger a recalculation of line numbers, so always call
 * words_recalc_line_numbers() after running this function.
 */
void words_move_line(gplayback_word_list *word_list, gplayback_word_id word_id,
                     gplayback_word_id dest_before, bool append_mode) {
  if (append_mode) {
    gplayback_word_id cursor = words_eol(word_list, word_id);

    do {
      words_move(word_list, cursor, dest_before, append_mode);
    } while ((cursor = words_prevl(word_list, cursor)) != 0);

  } else {
    gplayback_word_id cursor = words_bol(word_list, word_id);

    do {
      words_move(word_list, cursor, dest_before, append_mode);
    } while ((cursor = words_nextl(word_list, cursor)) != 0);
  }
}

/*
 * Moves the line containing word_id by move_amount lines. Does not
 * automatically trigger a recalculation of line numbers, so always call
 * words_recalc_line_numbers() after running this function.
 */
void words_move_line_absolute(gplayback_word_list *word_list,
                              gplayback_word_id word_id, int move_amount) {
  assert(move_amount != 0, "Move amount must be a non-zero integer");
  gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
  gplayback_word_id target_line =
      words_find_line(word_list, entry.item.line_idx + move_amount);

  if (target_line == 0) {
    words_move_line(word_list, word_id, words_last(word_list), true);
  } else if (move_amount > 0) {
    words_move_line(word_list, word_id, words_eol(word_list, target_line),
                    true);
  } else {
    words_move_line(word_list, word_id, target_line, false);
  }
}

void words_recalc_line_numbers(gplayback_word_list *word_list) {
  gplayback_word_id cursor = word_list->first;
  unsigned int line_idx = 0;

  while (cursor != 0) {
    gplayback_word_list_entry *entry =
        words_get_entry_pointer(word_list, cursor);
    entry->item.line_idx = line_idx;
    if (entry->item.len == 1 && entry->item.ptr[0] == '\n') {
      line_idx++;
    }

    cursor = words_next(word_list, cursor);
  }
}

void words_recalc_col_numbers(gplayback_word_list *word_list,
                              gplayback_word_id word_id) {
  gplayback_word_id cursor = words_bol(word_list, word_id);
  unsigned int col_idx = 0;
  do {
    gplayback_word_list_entry *entry =
        words_get_entry_pointer(word_list, cursor);
    entry->item.col_idx = col_idx;
    col_idx += entry->item.len;
  } while ((cursor = words_nextl(word_list, cursor)) != 0);
}

void words_recalc_col_numbers_by_line_idx(gplayback_word_list *word_list,
                                          unsigned int line_idx) {
  gplayback_word_id word_id = words_find_line(word_list, line_idx);
  return words_recalc_col_numbers(word_list, word_id);
}

char word_matchchr(char chr) {
  if (words_char_is_whitespace_or_newline(chr)) {
    return GPLAYBACK_TOKEN_SPACE;
  }
  return chr;
}

gplayback_word_list words_create_list(gplayback_slice text) {
  gplayback_word_list word_list = {0};
  word_list.entries.head = 1;
  memset(word_list.entries.items, 0, sizeof(word_list.entries.items));

  gplayback_word_list_entry *last_entry = NULL;
  int line_idx = 0;
  int col_idx = 0;
  const char *cursor = text.ptr;

  char lastchar = GPLAYBACK_TOKEN_NEVER;

  for (size_t i = 0; i < text.len; i++) {
    if ((!words_char_is_whitespace(text.ptr[i]) &&
         words_char_is_whitespace(lastchar)) ||
        lastchar == GPLAYBACK_TOKEN_NEWLINE ||
        text.ptr[i] == GPLAYBACK_TOKEN_NEWLINE) {

      if (i > 0) {
        // Commit existing word and begin new one
        unsigned int word_len = text.ptr + i - cursor;
        gplayback_word word = {.match = 0,
                               .line_idx = line_idx,
                               .col_idx = col_idx - word_len,
                               .len = word_len,
                               .ptr = cursor};
        gplayback_word_id word_id = words_append(&word_list, word, last_entry);
        last_entry = words_get_entry_pointer(&word_list, word_id);
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

  unsigned int word_len = text.ptr + text.len - cursor;
  gplayback_word word = {.match = 0,
                         .line_idx = line_idx,
                         col_idx - word_len,
                         .len = word_len,
                         .ptr = cursor};
  gplayback_word_id word_id = words_append(&word_list, word, last_entry);
  last_entry = words_get_entry_pointer(&word_list, word_id);

  return word_list;
}

int words_find_in_word(gplayback_word haystack, gplayback_word needle,
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

void words_transfer_slice(gplayback_word_list *word_list,
                          gplayback_slice from_slice,
                          gplayback_slice to_slice) {
  gplayback_word_id cursor = word_list->first;
  if (from_slice.ptr == to_slice.ptr && from_slice.len == to_slice.len) {
    dbg_log("Slices are identical, no need to transfer");
    return;
  }
  while (cursor != 0) {
    gplayback_word_list_entry *entry =
        words_get_entry_pointer(word_list, cursor);
    entry->item.ptr = to_slice.ptr + (entry->item.ptr - from_slice.ptr);
    cursor = entry->next;
  }
}

unsigned int words_find_anchors(gplayback_word_list *word_list,
                                gplayback_word_id *out) {
  gplayback_word_id cursor = word_list->first;
  unsigned int offset = 0;

  while (cursor != 0) {
    out[offset++] = cursor;
    cursor = words_next(word_list, words_eol(word_list, cursor));
  }

  return offset;
}

gplayback_word_id words_first(gplayback_word_list *word_list) {
  for (int i = 1; i < word_list->entries.head; i++) {
    if (word_list->entries.items[i].item.word_id != 0 &&
        word_list->entries.items[i].prev == 0) {
      return i;
    }
  }
  return 0;
}

void words_recalc_first(gplayback_word_list *word_list) {
  word_list->first = words_first(word_list);
}

unsigned int words_print_word(char *out, unsigned int out_len,
                              gplayback_word_list *word_list,
                              gplayback_word_id word_id) {
  unsigned int offset = 0;
  gplayback_word_list_entry entry = words_get_entry(word_list, word_id);
  for (int i = 0; i < entry.item.len; i++) {
    assert(offset < out_len, "Buffer overflow");
    out[offset++] = entry.item.ptr[i];
  }
  assert(offset < out_len, "Buffer overflow");
  out[offset++] = '\0';
  return offset;
}

unsigned int words_print_wordlist(char *out, unsigned int out_len,
                                  gplayback_word_list *word_list) {
  gplayback_word_id cursor = word_list->first;
  unsigned int offset = 0;
  while (cursor != 0) {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    for (int i = 0; i < entry.item.len; i++) {
      assert(offset < out_len, "Buffer overflow");
      out[offset++] = entry.item.ptr[i];
    }
    cursor = words_next(word_list, cursor);
  }
  assert(offset < out_len, "Buffer overflow");
  out[offset++] = '\0';
  return offset;
}

unsigned int words_print_line(char *out, unsigned int out_len,
                              gplayback_word_list *word_list,
                              gplayback_word_id word_id) {
  gplayback_word_id cursor = words_bol(word_list, word_id);
  unsigned int offset = 0;
  while (cursor != 0) {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    for (int i = 0; i < entry.item.len; i++) {
      assert(offset < out_len, "Buffer overflow");
      out[offset++] = entry.item.ptr[i];
    }
    cursor = words_nextl(word_list, cursor);
  }
  assert(offset < out_len, "Buffer overflow");
  out[offset++] = '\0';
  return offset;
}

unsigned int words_print_line_with_highlights(char *out, unsigned int out_len,
                                              gplayback_word_list *word_list,
                                              gplayback_word_id *word_ids,
                                              unsigned int word_ids_len,
                                              const char *normal_color,
                                              const char *highlight_color) {
  assert(word_ids_len > 0,
         "Can't print highlighted line with zero highlighted word IDs");
  gplayback_word_id start_id = word_ids[0];
  assert(start_id != 0, "Can't print highlighted line with zero start word ID");
  gplayback_word_id cursor = words_bol(word_list, word_ids[0]);
  unsigned int offset = 0;

  while (cursor != 0) {
    gplayback_word_list_entry entry = words_get_entry(word_list, cursor);
    bool highlight = false;
    for (int i = 0; i < word_ids_len; i++) {
      if (cursor == word_ids[i]) {
        highlight = true;
        break;
      }
    }

    if (highlight) {
      assert(offset < out_len, "Buffer overflow");
      offset += snprintf(out + offset, out_len - offset, "%.*s",
                         (int)strlen(highlight_color), highlight_color);
    }

    offset += snprintf(out + offset, out_len - offset, "%.*s", entry.item.len,
                       entry.item.ptr);

    if (highlight) {
      assert(offset < out_len, "Buffer overflow");
      offset += snprintf(out + offset, out_len - offset, "%.*s",
                         (int)strlen(normal_color), normal_color);
    }

    cursor = words_nextl(word_list, cursor);
  }
  assert(offset < out_len, "Buffer overflow");
  out[offset++] = '\0';
  return offset;
}

unsigned int words_span_count(gplayback_word_list *word_list,
                              gplayback_word_id first, gplayback_word_id last) {
  gplayback_word_id cursor = first;
  unsigned int count = 0;
  while (cursor != 0) {
    count++;
    cursor = cursor == last ? 0 : words_next(word_list, cursor);
  }
  return count;
}

bool words_identical(gplayback_word word_a, gplayback_word word_b) {
  if (word_a.len != word_b.len) {
    return false;
  }

  for (int i = 0; i < word_a.len; i++) {
    if (word_a.ptr[i] != word_b.ptr[i]) {
      return false;
    }
  }

  return true;
}
