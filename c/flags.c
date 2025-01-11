#include "flags.h"
#include "words.h"

bool flags_get(gplayback_flagset *flagset, unsigned int word_id,
               unsigned int flag) {
  assert(word_id != 0, "Can not get flag on word with zero ID");
  return ((*flagset)[word_id] & flag) != 0;
}

void flags_set(gplayback_flagset *flagset, unsigned int word_id,
               unsigned int flag, bool value) {
  assert(word_id != 0, "Can not set flag on word with zero ID");
  if (value) {
    (*flagset)[word_id] |= flag;
  } else {
    (*flagset)[word_id] &= ~flag;
  }
}

void flags_set_line(gplayback_word_list *word_list, gplayback_flagset *flagset,
                    unsigned int anchor_id, unsigned int flag, bool value) {
  unsigned int cursor = words_bol(word_list, anchor_id);
  while (cursor != 0) {
    flags_set(flagset, cursor, flag, value);
    cursor = words_nextl(word_list, cursor);
  }
}
