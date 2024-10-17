#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arrays.h"
#include "motions.h"

int word_subset_of_word(gplayback_word haystack, gplayback_word needle) {
  if (needle.len > haystack.len) {
    return -1;
  }

  int match_count = 0;
  for (int i = 0; i < haystack.len; i++) {
    if (needle.ptr[match_count] == haystack.ptr[i]) {
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

bool lines_identical(gplayback_line *a, gplayback_line *b) {
  if (a->char_len == 0 || b->char_len == 0 || a->char_len != b->char_len ||
      a->word_len != b->word_len) {
    return false;
  }
  if (a->words == NULL || b->words == NULL) {
    exit(1);
  }
  char *a_ptr = a->words->item.ptr;
  char *b_ptr = b->words->item.ptr;
  for (int i = 0; i < a->char_len; i++) {
    if (a_ptr[i] != b_ptr[i]) {
      return false;
    }
  }
  return true;
}

void match_lines(gplayback_lines *outer_lines, gplayback_lines *inner_lines) {
  for (int i = 0; i < outer_lines->count; i++) {
    gplayback_line *outer_line = &outer_lines->items[i];

    if (outer_line->linematched) {
      continue;
    }

    for (int j = 0; j < inner_lines->count; j++) {
      gplayback_line *inner_line = &inner_lines->items[j];

      if (inner_line->linematched) {
        continue;
      }

      if (!outer_line->dirty && !inner_line->dirty &&
          lines_identical(outer_line, inner_line)) {
        gplayback_word_list_entry *outer_word_cursor = outer_line->words;
        gplayback_word_list_entry *inner_word_cursor = inner_line->words;

        int outer_word_idx = 0;
        int inner_word_idx = 0;

        do {
          outer_word_cursor->item.match = inner_word_cursor;
          inner_word_cursor->item.match = outer_word_cursor;
        } while ((inner_word_cursor = inner_word_cursor->next) &&
                 (outer_word_cursor = outer_word_cursor->next) &&
                 (++outer_word_idx) < outer_line->word_len &&
                 (++inner_word_idx) < inner_line->word_len);

        outer_line->dirty = true;
        outer_line->linematched = true;
        inner_line->dirty = true;
        inner_line->linematched = true;

        break;
      }

      gplayback_word_list_entry *outer_word_cursor = outer_line->words;
      gplayback_word_list_entry *inner_word_cursor = inner_line->words;

      int outer_word_idx = 0;
      int inner_word_idx = 0;

      do {
        if (outer_word_cursor->item.match != NULL) {
          continue;
        }

        do {
          if (inner_word_cursor->item.match != NULL) {
            continue;
          }

          int match_idx = word_subset_of_word(inner_word_cursor->item,
                                              outer_word_cursor->item);

          if (match_idx < 0) {
            continue;
          }

          if (match_idx == 0 &&
              outer_word_cursor->item.len == inner_word_cursor->item.len) {
            outer_word_cursor->item.match = inner_word_cursor;
            inner_word_cursor->item.match = outer_word_cursor;

            outer_line->dirty = true;
            inner_line->dirty = true;
            break;
          }

          gplayback_word_list_entry *new_entry = inner_word_cursor->next;

          int outer_end = outer_word_cursor->item.len + match_idx;
          if (outer_end < inner_word_cursor->item.len) {
            gplayback_word word = {inner_word_cursor->item.ptr + outer_end,
                                   NULL, inner_word_cursor->item.line_idx,
                                   inner_word_cursor->item.len - outer_end};
            gplayback_word_list_entry *e =
                malloc(sizeof(gplayback_word_list_entry));
            e->item = word;
            e->next = new_entry;
            new_entry = e;
          }

          gplayback_word word = {
              inner_word_cursor->item.ptr + match_idx, outer_word_cursor,
              inner_word_cursor->item.line_idx, outer_word_cursor->item.len};

          if (match_idx > 0) {
            gplayback_word_list_entry *e =
                malloc(sizeof(gplayback_word_list_entry));
            e->item = word;
            e->next = new_entry;
            new_entry = e;

            inner_word_cursor->item.match = NULL;
            inner_word_cursor->item.len = match_idx;
            inner_word_cursor->item.line_idx = inner_word_cursor->item.line_idx;
            inner_word_cursor->next = new_entry;
            outer_word_cursor->item.match = new_entry;
          } else {
            inner_word_cursor->item = word;
            inner_word_cursor->next = new_entry;
            outer_word_cursor->item.match = inner_word_cursor;
          }

          outer_line->dirty = true;
          inner_line->dirty = true;
        } while ((inner_word_cursor = inner_word_cursor->next) &&
                 (++inner_word_idx) < inner_line->word_len);
      } while ((outer_word_cursor = outer_word_cursor->next) &&
               (++outer_word_idx) < outer_line->word_len);
    }
  }
}

gplayback_lines lines(gplayback_word_list word_list) {
  gplayback_lines lines = {0};
  gplayback_word_list_entry *entry = word_list.first;

  if (entry == NULL) {
    return lines;
  }

  int word_len = 0;
  int char_len = 0;
  int line_idx = 0;
  gplayback_word_list_entry *word_cursor = entry;
  do {
    if (entry->item.line_idx != line_idx) {
      gplayback_line line = {0};
      line.words = word_cursor;
      line.word_len = word_len;
      line.char_len = char_len;
      line.dirty = false;
      line.linematched = false;
      da_append(lines, line);

      word_len = 1;
      char_len = entry->item.len + entry->item.padlen;
      line_idx = entry->item.line_idx;
      word_cursor = entry;
    } else {
      word_len++;
      char_len += entry->item.len + entry->item.padlen;
    }
  } while ((entry = entry->next));

  gplayback_line line = {0};
  line.words = word_cursor;
  line.word_len = word_len;
  line.char_len = char_len;
  line.dirty = false;
  line.linematched = false;
  da_append(lines, line);

  return lines;
}

gplayback_segments
slice_text_by_newline_positions(gplayback_slice *text_slice, int cursor,
                                size_t text_limit,
                                gplayback_indices text_newlines) {
  gplayback_segments from_segments = {0};

  do {
    size_t from_line_limit = text_limit;
    for (int i = 0; i < text_newlines.count; i++) {
      size_t nl = text_newlines.items[i];
      if (nl >= cursor && nl < text_limit) {
        from_line_limit = nl + 1;
        break;
      }
    }

    gplayback_segment segment = {text_slice, cursor, from_line_limit - cursor};

    da_append(from_segments, segment);

    cursor = from_line_limit;
  } while (cursor < text_limit);

  return from_segments;
}

gplayback_word_list word_list(gplayback_slice text) {
  gplayback_word_list word_list = {0};
  gplayback_word_list_entry *last_entry = NULL;
  int line_idx = 0;
  int padlen = 0;
  char *cursor = NULL;

  for (size_t i = 0; i < text.len; i++) {
    bool is_newline = text.ptr[i] == '\n';
    if (is_newline || text.ptr[i] == ' ') {
      if (cursor != NULL) {
        gplayback_word_list_entry *entry =
            malloc(sizeof(gplayback_word_list_entry));

        entry->item.ptr = cursor;
        entry->item.line_idx = line_idx;
        entry->item.match = NULL;
        entry->item.padlen = padlen;
        entry->item.len = text.ptr + i - cursor;

        if (last_entry != NULL) {
          last_entry->next = entry;
        } else {
          word_list.first = entry;
        }
        last_entry = entry;

        cursor = NULL;
        padlen = 0;
      }

      if (is_newline) {
        line_idx++;
      } else {
        padlen++;
      }
    } else {
      if (cursor == NULL) {
        cursor = text.ptr + i;
      }
    }
  }
  return word_list;
}

void debug_log_lines(gplayback_word_list lhs_word_list,
                     gplayback_word_list rhs_word_list,
                     gplayback_lines lhs_lines, gplayback_lines rhs_lines) {
  gplayback_word_list_entry *w_e = lhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("lhs_word: (line %d, len %d) \"%.*s\"\n", word.line_idx, word.len,
           word.len, word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, len %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.len,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));

  w_e = rhs_word_list.first;
  do {
    gplayback_word word = w_e->item;
    printf("rhs_word: (line %d, len %d) \"%.*s\"\n", word.line_idx, word.len,
           word.len, word.ptr);
    if (word.match != NULL) {
      printf("   -> matched with: (line %d, len %d) \"%.*s\"\n",
             word.match->item.line_idx, word.match->item.len,
             word.match->item.len, word.match->item.ptr);
    }
  } while ((w_e = w_e->next));

  for (int i = 0; i < lhs_lines.count; i++) {
    gplayback_line line = lhs_lines.items[i];
    printf("lhs_line: %d (word len %zu, char len %zu)\n", i, line.word_len,
           line.char_len);
    gplayback_word_list_entry *word_cursor = line.words;
    int word_idx = 0;
    do {
      gplayback_word lhs_word = word_cursor->item;
      printf("  - lhs_word: %.*s\n", (int)lhs_word.len, lhs_word.ptr);
    } while ((word_cursor = word_cursor->next) && (++word_idx) < line.word_len);
  }

  for (int i = 0; i < rhs_lines.count; i++) {
    gplayback_line line = rhs_lines.items[i];
    printf("rhs_line: %d (word len %zu, char len %zu)\n", i, line.word_len,
           line.char_len);
    gplayback_word_list_entry *word_cursor = line.words;
    int word_idx = 0;
    do {
      gplayback_word rhs_word = word_cursor->item;
      printf("  - rhs_word: %.*s\n", (int)rhs_word.len, rhs_word.ptr);
    } while ((word_cursor = word_cursor->next) && (++word_idx) < line.word_len);
  }
}

gplayback_indices text_newline_indices(gplayback_slice text) {
  gplayback_indices newlines = {0};
  for (size_t i = 0; i < text.len; i++) {
    if (text.ptr[i] == '\n') {
      da_append(newlines, i);
    }
  }
  return newlines;
}

gplayback_vm_operations generate_operations(gplayback_slice lhs,
                                            gplayback_slice rhs,
                                            gplayback_diff diff[],
                                            size_t diff_len) {
  gplayback_vm_operations operations = {0};

  gplayback_word_list lhs_word_list = word_list(lhs);
  gplayback_word_list rhs_word_list = word_list(rhs);

  gplayback_lines lhs_lines = lines(lhs_word_list);
  gplayback_lines rhs_lines = lines(rhs_word_list);

  printf("Before matching:\n");
  debug_log_lines(lhs_word_list, rhs_word_list, lhs_lines, rhs_lines);

  match_lines(&lhs_lines, &rhs_lines);
  match_lines(&rhs_lines, &lhs_lines);

  // Delete rows
  for (int i = 0; i < lhs_lines.count; i++) {
    gplayback_line line = lhs_lines.items[i];
    if (line.dirty) {
      continue;
    }

    gplayback_vm_op_delete_row *operation =
        malloc(sizeof(gplayback_vm_op_delete_row));
    operation->line = i;

    gplayback_vm_operation op = {GPLAYBACK_OP_DELETE_ROW, operation};

    da_append(operations, op);
  }

  printf("After matching:\n");
  debug_log_lines(lhs_word_list, rhs_word_list, lhs_lines, rhs_lines);

  /*gplayback_word_list_entry *lhs_word = lhs_word_list.first;*/
  /*do {*/
  /*  gplayback_word_list_entry *rhs_word = rhs_word_list.first;*/
  /*  do {*/
  /*    int match_idx = word_subset_of_word(lhs_word->item, rhs_word->item);*/
  /*    if (match_idx == -1) {*/
  /*      continue;*/
  /*    }*/
  /**/
  /*  } while ((rhs_word = rhs_word->next));*/
  /*} while ((lhs_word = lhs_word->next));*/
  /**/
  /*gplayback_indices lhs_newlines = text_newline_indices(lhs);*/
  /*gplayback_indices rhs_newlines = text_newline_indices(rhs);*/
  /**/
  /*for (int i = 0; i < diff_len; i++) {*/
  /*  gplayback_diff current_diff = diff[i];*/
  /**/
  /*  size_t lhs_cursor = current_diff.from_file_char_start;*/
  /*  size_t lhs_limit =*/
  /*      current_diff.from_file_char_start + current_diff.from_file_char_len;*/
  /**/
  /*  size_t rhs_cursor = current_diff.to_file_char_start;*/
  /*  size_t rhs_limit =*/
  /*      current_diff.to_file_char_start + current_diff.to_file_char_len;*/
  /**/
  /*  if (lhs_limit > lhs.len) {*/
  /*    fprintf(stderr,*/
  /*            "Error: diff span (%zu) is greater than lhs text len (%zu)\n",*/
  /*            lhs_limit, lhs.len);*/
  /*    exit(1);*/
  /*  }*/
  /**/
  /*  if (rhs_limit > rhs.len) {*/
  /*    fprintf(stderr,*/
  /*            "Error: diff span (%zu) is greater than rhs text len (%zu)\n",*/
  /*            rhs_limit, rhs.len);*/
  /*    exit(1);*/
  /*  }*/
  /**/
  /*  gplayback_segments lhs_segments = slice_text_by_newline_positions(*/
  /*      &lhs, lhs_cursor, lhs_limit, lhs_newlines);*/
  /**/
  /*  gplayback_segments rhs_segments = slice_text_by_newline_positions(*/
  /*      &rhs, rhs_cursor, rhs_limit, rhs_newlines);*/
  /**/
  /*  int cursor = 0;*/
  /**/
  /*  for (int j = 0; j < rhs_segments.count; j++) {*/
  /*    for (int k = cursor; k < lhs_segments.count; k++) {*/
  /*      gplayback_segment from_segment = lhs_segments.items[k];*/
  /*      gplayback_segment to_segment = rhs_segments.items[j];*/
  /**/
  /*      gplayback_segments changed = {0};*/
  /**/
  /*      double token_value = 0;*/
  /*      double total_token_value = 0;*/
  /*      int changed_len = 0;*/
  /*      int changed_cur = 0;*/
  /**/
  /*      gplayback_vm_operation *insert_at_end = NULL;*/
  /*      gplayback_vm_operation *delete_at_end = NULL;*/
  /**/
  /*      int min_len;*/
  /*      if (from_segment.len > to_segment.len) {*/
  /*        min_len = to_segment.len;*/
  /*      } else {*/
  /*        min_len = from_segment.len;*/
  /*      }*/
  /**/
  /*      for (int l = 0; l < min_len; l++) {*/
  /*        if (l >= from_segment.len) {*/
  /*          insert_at_end = malloc(sizeof(gplayback_vm_operation));*/
  /*          insert_at_end->operation = GPLAYBACK_OP_INSERT;*/
  /*          insert_at_end->operand1 = lhs_cursor + l;*/
  /*          insert_at_end->operand2 = rhs_cursor + l;*/
  /*          insert_at_end->operand3 = to_segment.len - rhs_cursor - l;*/
  /*          break;*/
  /*        }*/
  /*        if (l >= to_segment.len) {*/
  /*          delete_at_end = malloc(sizeof(gplayback_vm_operation));*/
  /*          delete_at_end->operation = GPLAYBACK_OP_DELETE;*/
  /*          delete_at_end->operand1 = lhs_cursor + l;*/
  /*          delete_at_end->operand2 = from_segment.len - lhs_cursor - l;*/
  /*          break;*/
  /*        }*/
  /*        if (segmentptr(from_segment)[l] == segmentptr(to_segment)[l]) {*/
  /*          if (token_value == 0) {*/
  /*            token_value = GPLAYBACK_REPLACE_SIM_ENTROPY;*/
  /*          } else {*/
  /*            token_value *= GPLAYBACK_REPLACE_SIM_ENTROPY;*/
  /*          }*/
  /*          if (changed_len > 0) {*/
  /*            gplayback_segment changed_seg =*/
  /*                subsegment(from_segment, changed_cur, changed_len);*/
  /*            da_append(changed, changed_seg);*/
  /*            changed_len = 0;*/
  /*          }*/
  /*        } else {*/
  /*          total_token_value += token_value;*/
  /*          token_value = 0;*/
  /*          if (changed_len == 0) {*/
  /*            changed_cur = l;*/
  /*          }*/
  /*          changed_len++;*/
  /*        }*/
  /*      }*/
  /**/
  /*      total_token_value += token_value;*/
  /*      if (changed_len > 0) {*/
  /*        gplayback_segment changed_seg =*/
  /*            subsegment(from_segment, changed_cur, changed_len);*/
  /*        da_append(changed, changed_seg);*/
  /*        changed_len = 0;*/
  /*      }*/
  /**/
  /*      double similarity_score =*/
  /*          (double)total_token_value /*/
  /*          pow(GPLAYBACK_REPLACE_SIM_ENTROPY, from_segment.len);*/
  /**/
  /*      if (similarity_score >= GPLAYBACK_REPLACE_SIM_TRESHOLD) {*/
  /*        if (similarity_score < 1) {*/
  /*          for (int l = 0; l < changed.count; l++) {*/
  /*            gplayback_segment changed_seg = changed.items[l];*/
  /**/
  /*            gplayback_vm_operation operation = {*/
  /*                GPLAYBACK_OP_REPLACE, changed_seg.start, changed_seg.len,*/
  /*                rhs_cursor, to_segment.len};*/
  /*            da_append(operations, operation);*/
  /*          }*/
  /*          cursor = k + 1;*/
  /*        }*/
  /*        goto end;*/
  /*      }*/
  /*    }*/
  /*  }*/
  /*end:*/
  /**/
  /*  for (int j = 0; j < lhs_segments.count; j++) {*/
  /*    gplayback_segment segment = lhs_segments.items[j];*/
  /*    printf("--- %.*s", (int)segment.len, segment.slice->ptr);*/
  /*  }*/
  /**/
  /*  for (int j = 0; j < rhs_segments.count; j++) {*/
  /*    gplayback_segment segment = rhs_segments.items[j];*/
  /*    printf("+++ %.*s", (int)segment.len, segment.slice->ptr);*/
  /*  }*/
  /**/
  /*  char *from_file_start = lhs.ptr + current_diff.from_file_char_start;*/
  /*  char *to_file_start = rhs.ptr + current_diff.to_file_char_start;*/
  /**/
  /*  size_t cursor_from = 0;*/
  /*  size_t cursor_to = 0;*/
  /*}*/

  return operations;
}
