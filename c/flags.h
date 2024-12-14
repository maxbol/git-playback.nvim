#ifndef FLAGS_H
#define FLAGS_H

#include <stdbool.h>
#include <stdlib.h>

#include "assert.h"
#include "words.h"

#define FLAG_VISITED 1 << 0
#define FLAG_INSERT_PROCESSED 1 << 1

typedef unsigned int gplayback_flags;
typedef gplayback_flags gplayback_flagset[WORD_LINE_MAX_LEN];

void flags_set(gplayback_flagset *flagset, unsigned int word_id,
               unsigned int flag, bool value);
bool flags_get(gplayback_flagset *flagset, unsigned int word_id,
               unsigned int flag);
void flags_set_line(gplayback_word_list *word_list, gplayback_flagset *flagset,
                    unsigned int anchor_id, unsigned int flag, bool value);

#endif // !FLAGS_H
