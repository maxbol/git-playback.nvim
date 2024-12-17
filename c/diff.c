#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "assert.h"
#include "constants.h"
#include "diff.h"
#include "log.h"
#include "slice.h"
#include "words.h"
#include "writestr.h"

char *diff_debug(gplayback_diff *diff) {
  gplayback_word_list *lhs_words = &diff->lhs.words;
  gplayback_word_list *rhs_words = &diff->rhs.words;

  gplayback_writestr_state ws = writestr_create(512);

  writestr(ws, "Movelines:\n");
  gplayback_word_id cursor = rhs_words->first;
  while (cursor != 0) {
    gplayback_diff_moveline moveline = diff->movelines[cursor];

    if (moveline.lhs_anchor == 0) {
      cursor = words_next(rhs_words, cursor);
      continue;
    }

    writestr(ws, "\t- Moving %d lines...\n", moveline.lines_amount);

    gplayback_word_list_entry target_entry = words_get_entry(rhs_words, cursor);
    gplayback_word_list_entry anchor_entry =
        words_get_entry(lhs_words, moveline.lhs_anchor);

    {
      char buf[WORD_LINE_MAX_LEN] = {0};
      char escaped[WORD_LINE_MAX_LEN * 2] = {0};
      unsigned int len = words_print_line(buf, WORD_LINE_MAX_LEN, lhs_words,
                                          moveline.lhs_anchor);
      unsigned int escaped_len =
          escape_fmt(escaped, WORD_LINE_MAX_LEN * 2, "%.*s", len, buf);
      writestr(ws, "\t\t>> Starting from LHS line #%d: \e[1;32m%.*s\e[0m\n",
               anchor_entry.item.line_idx, escaped_len, escaped);
    }

    {
      char buf[WORD_LINE_MAX_LEN] = {0};
      char escaped[WORD_LINE_MAX_LEN * 2] = {0};
      unsigned int len =
          words_print_line(buf, WORD_LINE_MAX_LEN, rhs_words, cursor);
      unsigned int escaped_len =
          escape_fmt(escaped, WORD_LINE_MAX_LEN * 2, "%.*s", len, buf);
      writestr(ws,
               "\t\t>> Moving to before RHS line #%d at column #%d: "
               "\e[1;32m%.*s\e[0m\n",
               target_entry.item.line_idx, target_entry.item.col_idx,
               escaped_len, escaped);
    }

    cursor = words_next(rhs_words, cursor);
  }
  writestr(ws, "\n");

  writestr(ws, "Movewords:\n");
  cursor = rhs_words->first;
  while (cursor != 0) {
    gplayback_diff_movewords movewords = diff->movewords[cursor];

    if (movewords.lhs_start == 0) {
      cursor = words_next(rhs_words, cursor);
      continue;
    }

    gplayback_word_list_entry target_entry = words_get_entry(rhs_words, cursor);
    gplayback_word_list_entry start_entry =
        words_get_entry(lhs_words, movewords.lhs_start);

    writestr(ws, "\t- Moving %d words...\n", movewords.words_amount);
    gplayback_word_id move_word_ids[128] = {0};

    gplayback_word_id mv_cursor = movewords.lhs_start;
    unsigned int i = movewords.words_amount;
    while (mv_cursor != 0 && i-- > 0) {
      move_word_ids[i] = mv_cursor;
      mv_cursor = words_next(lhs_words, mv_cursor);
    }

    {
      char buf[WORD_LINE_MAX_LEN] = {0};
      char escaped[WORD_LINE_MAX_LEN * 2] = {0};
      unsigned int len = words_print_line_with_highlights(
          buf, WORD_LINE_MAX_LEN, lhs_words, move_word_ids,
          movewords.words_amount, "\e[0;32m", "\e[3;32m");
      unsigned int escaped_len =
          escape_fmt(escaped, WORD_LINE_MAX_LEN * 2, "%.*s", len, buf);
      writestr(ws,
               "\t\t>> Moving LHS words (line %d, col %d, start id %d): "
               "\e[0;32m%.*s\e[0m\n",
               start_entry.item.line_idx, start_entry.item.col_idx,
               start_entry.item.word_id, escaped_len, escaped);
    }

    {
      char buf[WORD_LINE_MAX_LEN] = {0};
      char escaped[WORD_LINE_MAX_LEN * 2] = {0};
      unsigned int len =
          words_print_line_with_highlights(buf, WORD_LINE_MAX_LEN, rhs_words,
                                           &cursor, 1, "\e[0;32m", "\e[3;32m");
      unsigned int escaped_len =
          escape_fmt(escaped, WORD_LINE_MAX_LEN * 2, "%.*s", len, buf);
      writestr(ws,
               "\t\t\t>> Instead of inserting RHS word (line %d, col %d): "
               "\e[0;32m%.*s\e[0m\n",
               target_entry.item.line_idx, target_entry.item.col_idx,
               escaped_len, escaped);
    }

    cursor = words_next(rhs_words, cursor);
  }

  writestr(ws, "\n");

  writestr(ws, "LHS word list:\n");
  char *lhs_wl_debug = words_allocprint_word_list("LHS", lhs_words, rhs_words);
  writestr(ws, "%.*s", (int)strlen(lhs_wl_debug), lhs_wl_debug);
  writestr(ws, "\n");
  free(lhs_wl_debug);

  writestr(ws, "RHS word list:\n");
  char *rhs_wl_debug = words_allocprint_word_list("RHS", rhs_words, lhs_words);
  writestr(ws, "%.*s", (int)strlen(rhs_wl_debug), rhs_wl_debug);
  writestr(ws, "\n");
  free(rhs_wl_debug);

  return ws.out;
}

void diff_free(gplayback_diff *diff) {
  slice_free_buf(diff->lhs.slice);
  slice_free_buf(diff->rhs.slice);
}

gplayback_text diff_clone_text(gplayback_text text) {
  gplayback_text clone = {0};
  clone.slice = slice_copy(text.slice);
  clone.words = text.words;
  words_transfer_slice(&clone.words, text.slice, clone.slice);
  return clone;
}

gplayback_diff diff_clone(gplayback_diff *diff) {
  gplayback_diff clone = {0};

  // Clone LHS text
  clone.lhs = diff_clone_text(diff->lhs);

  // Clone RHS texts
  clone.rhs = diff_clone_text(diff->rhs);

  // Clone movelines
  gplayback_word_id cursor = diff->rhs.words.first;
  while (cursor != 0) {
    clone.movelines[cursor] = diff->movelines[cursor];
    cursor = words_next(&diff->rhs.words, cursor);
  }

  // Clone movewords
  cursor = diff->rhs.words.first;
  while (cursor != 0) {
    clone.movewords[cursor] = diff->movewords[cursor];
    cursor = words_next(&diff->rhs.words, cursor);
  }

  return clone;
}

// TODO(2024-12-10, Max Bolotin): Processing a line for move operations can have
// three outcomes:
//  - a. More than X% of the words have matches, and they all match words on the
//  same RHS line. This should result in a move line operation that is run on
//  RHS catchup and inserts the line before the anchor word of the next RHS line
//  is inserted. The line to be moved is the entire line, including a trailing
//  line separators if that exists.
//  - b. There are several word matches that all point to the same RHS line, but
//  the percentage of matches is less than X%. Result: Slip into individual
//  word selection mode (c-d).
//  - c. Iterate over the words. There is a set of words with matches, but their
//  RHS matches have differing line indices. This leads to a series of MOVE
//  WORDS operations. To qualify for moving, more than X words in a row must
//  match. Individual line separators are automatically disqualified for moving.
//  - d. There is a set of words with matches, with differing RHS line indices,
//  but the amount of words in the set is lower than minimum amount of words in
//  a row required for a MOVE ROW. Result: unmatch all words and their RHS
//  counterparts.
//
//  How do we track COPY ROW and COPY WORDS as part of this? Would be great to
//  have some easy way to precheck multiple match candidates here, but that
//  groundwork probably has to be laid elsewhere. (Copies should only be made in
//  the case the same LHS text data is useful in multiple places in the RHS
//  text)
//

void diff_process_movewords(gplayback_word_list *lhs_words,
                            gplayback_word_list *rhs_words,
                            gplayback_word_id *anchors,
                            unsigned int anchors_len,
                            gplayback_generate_diff_opts opts,
                            gplayback_diff_movewords *movewords,
                            gplayback_diff_moveline *movelines) {
  int tail_rhs_pos = -1;

  gplayback_diff_moveline *current_moveline = NULL;

  for (int i = 0; i <= anchors_len; i++) {
    gplayback_word_id cursor = anchors[i];
    unsigned int word_count = 0;
    unsigned int matched_count = 0;

    gplayback_word_id selection_start = 0;
    unsigned int last_rhs_line = 0;
    gplayback_word_id last_cursor = 0;
    gplayback_word_id last_target = 0;
    bool is_single_rhs_linematch = true;

    gplayback_diff_word_process_span spans_to_process[4096] = {0};
    unsigned int spans_to_process_len = 0;

    while (cursor != 0) {
      gplayback_word_list_entry entry = words_get_entry(lhs_words, cursor);
      dbg_log("Processing word #%d: %.*s", entry.item.word_id, entry.item.len,
              entry.item.ptr);

      if (words_is_linesep(entry.item)) {
        cursor = words_nextl(lhs_words, cursor);
        continue;
      }

      word_count++;

      if (entry.item.match == 0) {
        dbg_log("Word is unmatched");
        if (selection_start != 0) {
          gplayback_diff_word_process_span span = {.from = selection_start,
                                                   .to = last_cursor,
                                                   .target = last_target};
          assert(spans_to_process_len < 4096, "Too many spans to process");
          spans_to_process[spans_to_process_len++] = span;
          dbg_log("\e[0;32mAdded span from %d to %d, target %d\e[0m", span.from,
                  span.to, span.target);
        }
        selection_start = 0;
        last_rhs_line = 0;
        last_target = 0;
        last_cursor = cursor;
        cursor = words_nextl(lhs_words, cursor);
        continue;
      }

      matched_count++;

      gplayback_word_list_entry match =
          words_get_entry(rhs_words, entry.item.match);

      int match_pos =
          (WORD_LINE_MAX_LEN * match.item.line_idx) + match.item.col_idx;

      if (tail_rhs_pos == -1) {
        tail_rhs_pos = match_pos;
      }

      if (match_pos >= tail_rhs_pos) {

        dbg_log("Matches word that is on a line below ((%d)) current tail "
                "((%d)) (greater or "
                "equal line idx) - closing existing move spans, and "
                "adding no new ones",
                match_pos, tail_rhs_pos);

        if (selection_start != 0) {
          gplayback_diff_word_process_span span = {.from = selection_start,
                                                   .to = last_cursor,
                                                   .target = last_target};
          assert(spans_to_process_len < 4096, "Too many spans to process");
          spans_to_process[spans_to_process_len++] = span;

          dbg_log("\e[0;32mAdded span from %d to %d, target %d\e[0m", span.from,
                  span.to, span.target);
        }
        selection_start = 0;
        last_rhs_line = 0;
        last_target = 0;
        tail_rhs_pos = match_pos;
        last_cursor = cursor;
        cursor = words_nextl(lhs_words, cursor);
        continue;
      }

      dbg_log(
          "Matches word that is on a line above ((%d)) (lower line idx) the "
          "current tail ((%d)) - creating or adding to existing move span",
          match_pos, tail_rhs_pos);

      if (selection_start == 0) {
        selection_start = cursor;
        last_target = entry.item.match;
      } else if (last_rhs_line != 0 && last_rhs_line != match.item.line_idx) {

        dbg_log("Matches word that is on a different line than earlier matches "
                "- closing existing move span, and adding a new one");

        gplayback_diff_word_process_span span = {
            .from = selection_start, .to = last_cursor, .target = last_target};
        assert(spans_to_process_len < 4096, "Too many spans to process");
        spans_to_process[spans_to_process_len++] = span;

        dbg_log("\e[0;32mAdded span from %d to %d, target %d\e[0m", span.from,
                span.to, span.target);

        is_single_rhs_linematch = false;
        selection_start = cursor;
        last_target = entry.item.match;
      }

      last_rhs_line = match.item.line_idx;
      last_cursor = cursor;
      cursor = words_nextl(lhs_words, cursor);
    }

    const float entropy = 1 - (float)matched_count / word_count;

    if (last_target != 0 && is_single_rhs_linematch &&
        entropy <= opts.moveline_entropy_treshold) {

      gplayback_word_list_entry target_entry =
          words_get_entry(rhs_words, last_target);
      gplayback_word_list_entry anchor_entry =
          words_get_entry(lhs_words, anchors[i]);

      if (target_entry.item.line_idx != anchor_entry.item.line_idx) {
        dbg_log("Treating as single line move operation");

        if (current_moveline == NULL) {
          current_moveline = &movelines[words_next(
              rhs_words, words_eol(rhs_words, last_target))];
          current_moveline->lhs_anchor = anchors[i];
          current_moveline->lines_amount = 0;
        }
        current_moveline->lines_amount++;
        continue;
      }
    }
    current_moveline = NULL;

    dbg_log("Treating as multi word move operation");

    if (selection_start != 0) {

      dbg_log("Closing last move span");

      gplayback_diff_word_process_span span = {
          .from = selection_start, .to = last_cursor, .target = last_target};
      assert(spans_to_process_len < 4096, "Too many spans to process");
      spans_to_process[spans_to_process_len++] = span;

      dbg_log("\e[0;32mAdded span from %d to %d, target %d\e[0m", span.from,
              span.to, span.target);
    }

    dbg_log("Processing %d spans", spans_to_process_len);

    for (int i = 0; i < spans_to_process_len; i++) {
      gplayback_diff_word_process_span span = spans_to_process[i];
      unsigned int word_count = words_span_count(lhs_words, span.from, span.to);

      dbg_log("\e[0;32mProcessing span from %d to %d, target %d, word count "
              "%d\e[0m",
              span.from, span.to, span.target, word_count);

      gplayback_word_list_entry *entry =
          words_get_entry_pointer(lhs_words, span.from);

      if (word_count < opts.moveword_min_word_amount) {
        gplayback_word_list_entry *match =
            words_get_entry_pointer(rhs_words, entry->item.match);
        entry->item.match = 0;
        match->item.match = 0;

        continue;
      }

      movewords[span.target] = (gplayback_diff_movewords){
          .lhs_start = span.from, .words_amount = word_count};
    }
  }
}

void diff_match_lines(gplayback_text *outer, gplayback_text *inner) {
  gplayback_word_list *outer_words = &outer->words;
  gplayback_word_list *inner_words = &inner->words;

  char(*outer_line_char)[WORD_MAX_LINES] =
      malloc(sizeof(char[WORD_LINE_MAX_LEN][WORD_MAX_LINES]));
  char(*inner_line_char)[WORD_MAX_LINES] =
      malloc(sizeof(char[WORD_LINE_MAX_LEN][WORD_MAX_LINES]));

  gplayback_word_id outer_line_anchor[WORD_MAX_LINES];
  gplayback_word_id inner_line_anchor[WORD_MAX_LINES];

  unsigned int outer_line_size[WORD_MAX_LINES];
  unsigned int inner_line_size[WORD_MAX_LINES];

  unsigned int outer_line_offset = 0;
  unsigned int inner_line_offset = 0;

  gplayback_word_id outer_cursor = outer_words->first;
  while (outer_cursor != 0) {
    assert(outer_cursor < WORD_MAX_LINES, "Too many lines in outer text");
    outer_line_anchor[outer_line_offset] = outer_cursor;
    outer_line_size[outer_line_offset] =
        words_print_line(outer_line_char[outer_line_offset], WORD_LINE_MAX_LEN,
                         outer_words, outer_cursor);
    outer_line_offset++;
    outer_cursor =
        words_next(outer_words, words_eol(outer_words, outer_cursor));
  }

  gplayback_word_id inner_cursor = inner_words->first;
  while (inner_cursor != 0) {
    assert(inner_cursor < WORD_MAX_LINES, "Too many lines in inner text");
    inner_line_anchor[inner_line_offset] = inner_cursor;
    inner_line_size[inner_line_offset] =
        words_print_line(inner_line_char[inner_line_offset], WORD_LINE_MAX_LEN,
                         inner_words, inner_cursor);
    inner_line_offset++;
    inner_cursor =
        words_next(inner_words, words_eol(inner_words, inner_cursor));
  }

  for (int i = 0; i < outer_line_offset; i++) {
    unsigned int outer_size = outer_line_size[i];
    char *outer_line = outer_line_char[i];

    for (int j = 0; j < inner_line_offset; j++) {
      unsigned int inner_size = inner_line_size[j];
      char *inner_line = inner_line_char[j];

      if (outer_size != inner_size ||
          strncmp(outer_line, inner_line, outer_size) != 0) {
        continue;
      }

      gplayback_word_id outer_cursor = outer_line_anchor[i];
      gplayback_word_id inner_cursor = inner_line_anchor[j];

      while (outer_cursor != 0 && inner_cursor != 0) {
        gplayback_word_list_entry *outer_anchor_entry =
            words_get_entry_pointer(outer_words, outer_cursor);
        gplayback_word_list_entry *inner_anchor_entry =
            words_get_entry_pointer(inner_words, inner_cursor);

        outer_anchor_entry->item.match = inner_cursor;
        inner_anchor_entry->item.match = outer_cursor;

        outer_cursor = words_nextl(outer_words, outer_cursor);
        inner_cursor = words_nextl(inner_words, inner_cursor);
      }
    }
  }

  free(outer_line_char);
  free(inner_line_char);
}

void diff_match_words(gplayback_text *outer, gplayback_text *inner) {
  gplayback_word_list *outer_words = &outer->words;
  gplayback_word_list *inner_words = &inner->words;

  gplayback_word_id outer_cursor = outer_words->first;

  while (outer_cursor != 0) {
    gplayback_word_list_entry *outer_entry =
        words_get_entry_pointer(outer_words, outer_cursor);

    if (words_is_whitespace(
            outer_entry->item) /*|| words_is_linesep(outer_entry->item)*/) {
      outer_cursor = outer_entry->next;
      continue;
    }

    if (outer_entry->item.match != 0) {
      outer_cursor = outer_entry->next;
      continue;
    }

    bool outer_strict =
        words_char_is_whitespace_or_newline(outer_entry->item.ptr[0]);

    gplayback_word_id inner_cursor = inner_words->first;

    while (inner_cursor != 0) {
      gplayback_word_list_entry *inner_entry =
          words_get_entry_pointer(inner_words, inner_cursor);

      if (words_is_whitespace(
              inner_entry->item) /*|| words_is_linesep(inner_entry->item)*/) {
        inner_cursor = inner_entry->next;
        continue;
      }

      if (inner_entry->item.match != 0) {
        inner_cursor = inner_entry->next;
        continue;
      }

      bool strict = outer_strict || words_char_is_whitespace_or_newline(
                                        inner_entry->item.ptr[0]);

      int match_idx =
          words_find_in_word(inner_entry->item, outer_entry->item, strict);

      if (match_idx < 0) {
        inner_cursor = inner_entry->next;
        continue;
      }

      if (match_idx == 0 && outer_entry->item.len == inner_entry->item.len) {
        outer_entry->item.match = inner_cursor;
        inner_entry->item.match = outer_cursor;
        break;
      }

      int outer_end = match_idx + outer_entry->item.len;

      gplayback_word_list_entry *next =
          words_get_entry_pointer(inner_words, inner_entry->next);

      if (outer_end < inner_entry->item.len) {
        gplayback_word word = {.match = 0,
                               .line_idx = inner_entry->item.line_idx,
                               .col_idx = inner_entry->item.col_idx + outer_end,
                               .len = inner_entry->item.len - outer_end,
                               .ptr = inner_entry->item.ptr + outer_end};
        gplayback_word_id word_id = words_create_entry(inner_words, word);
        gplayback_word_list_entry *entry =
            words_get_entry_pointer(inner_words, word_id);
        entry->next = next->item.word_id;
        next->prev = word_id;
        next = entry;
      }

      gplayback_word word = {.match = outer_cursor,
                             .line_idx = inner_entry->item.line_idx,
                             .col_idx = inner_entry->item.col_idx + match_idx,
                             .len = outer_entry->item.len,
                             .ptr = inner_entry->item.ptr + match_idx};

      if (match_idx > 0) {
        gplayback_word_id word_id = words_create_entry(inner_words, word);
        gplayback_word_list_entry *entry =
            words_get_entry_pointer(inner_words, word_id);
        entry->next = next->item.word_id;
        entry->prev = inner_cursor;
        next->prev = word_id;
        next = entry;

        inner_entry->item.match = 0;
        inner_entry->item.len = match_idx;
        outer_entry->item.match = word_id;
      } else {
        word.word_id = inner_entry->item.word_id;
        inner_entry->item = word;
        next->prev = inner_cursor;
        outer_entry->item.match = inner_cursor;
      }

      inner_entry->next = next->item.word_id;

      break;
    }

    outer_cursor = outer_entry->next;
  }
}

gplayback_diff diff_generate(gplayback_slice lhs, gplayback_slice rhs,
                             gplayback_generate_diff_opts opts) {
  gplayback_diff diff = {0};

  diff.lhs.slice = slice_copy(lhs);
  diff.rhs.slice = slice_copy(rhs);

  diff.lhs.words = words_create_list(diff.lhs.slice);
  diff.rhs.words = words_create_list(diff.rhs.slice);

  {
    char *lhs_wl_debug =
        words_allocprint_word_list("LHS", &diff.lhs.words, &diff.rhs.words);
    printf("LHS word list:\n%.*s", (int)strlen(lhs_wl_debug), lhs_wl_debug);
    free(lhs_wl_debug);
  }
  {
    char *rhs_wl_debug =
        words_allocprint_word_list("RHS", &diff.rhs.words, &diff.lhs.words);
    printf("RHS word list:\n%.*s", (int)strlen(rhs_wl_debug), rhs_wl_debug);
    free(rhs_wl_debug);
  }

  // Primacy is given to identically matched lines
  diff_match_lines(&diff.lhs, &diff.rhs);
  diff_match_lines(&diff.rhs, &diff.lhs);

  // Then we try to match words individually
  diff_match_words(&diff.lhs, &diff.rhs);
  diff_match_words(&diff.rhs, &diff.lhs);

  gplayback_word_id anchors[WORD_MAX_LINES];
  unsigned int anchors_len = words_find_anchors(&diff.lhs.words, anchors);

  diff_process_movewords(&diff.lhs.words, &diff.rhs.words, anchors, anchors_len,
                         opts, diff.movewords, diff.movelines);

  return diff;
}
