#ifndef PATCH_H
#define PATCH_H

#include "diff.h"
#include "segments.h"
#include "words.h"

typedef struct {
  int line;
  int column;
  /*size_t chr;*/
} gplayback_cursorpos;

typedef struct {
  int type;
  void *data;
  gplayback_cursorpos cursor;
} gplayback_vm_operation;

typedef struct gplayback_vm_operation_entry {
  gplayback_vm_operation item;
  struct gplayback_vm_operation_entry *next;
  struct gplayback_vm_operation_entry *prev;
} gplayback_vm_operation_entry;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_word_after;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_row_after;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_word_before;

typedef struct {
  gplayback_slice src;
} gplayback_vm_op_insert_row_before;

typedef struct {
  size_t char_len;
} gplayback_vm_op_delete_words;

typedef struct {
  size_t no_of_lines;
} gplayback_vm_op_delete_rows;

typedef struct {
  size_t no_of_lines;
  int move_amount;
} gplayback_vm_op_move_rows;

typedef struct {
  gplayback_vm_operation_entry *first;
} gplayback_patch;

typedef struct {
  int anchor_line;
  int lines_amount;
} deleteset;

typedef struct {
  gplayback_word_list_entry *anchor;
  int lines_amount;
  int move_amount;
} moveset;

char *debug_patch(gplayback_patch patch);
void free_operation_entry(gplayback_vm_operation_entry *entry);
void free_patch(gplayback_patch patch);
gplayback_patch generate_patch(gplayback_diff *diff);

#endif // !PATCH_H
