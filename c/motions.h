#include "segments.h"
#include <stdio.h>

#define GPLAYBACK_DIFF_APPEND 1
#define GPLAYBACK_DIFF_DELETE 2
#define GPLAYBACK_DIFF_CHANGE 3

#define GPLAYBACK_OP_INSERT_WORD_AFTER 1
#define GPLAYBACK_OP_INSERT_ROW_AFTER 2
#define GPLAYBACK_OP_DELETE_WORD 3
#define GPLAYBACK_OP_DELETE_ROW 4
#define GPLAYBACK_OP_MOVE_ROW 5

#define GPLAYBACK_REPLACE_SIM_ENTROPY 1.5
#define GPLAYBACK_REPLACE_SIM_TRESHOLD 0.25

typedef struct gplayback_word {
  char *ptr;
  struct gplayback_word_list_entry *match;
  int line_idx;
  int len;
  int padlen;
} gplayback_word;

typedef struct gplayback_word_list_entry {
  gplayback_word item;
  struct gplayback_word_list_entry *next;
} gplayback_word_list_entry;

typedef struct {
  gplayback_word_list_entry *first;
} gplayback_word_list;

typedef struct {
  gplayback_word_list_entry *words;
  size_t word_len;
  size_t char_len;
  bool dirty;
  bool linematched;
} gplayback_line;

typedef struct {
  int from_file_line_start;
  int to_file_line_start;
  size_t from_file_char_start;
  size_t from_file_char_len;
  size_t to_file_char_start;
  size_t to_file_char_len;
} gplayback_diff;

typedef struct {
  int type;
  void *data;
} gplayback_vm_operation;

typedef struct {
  size_t line;
  size_t column;
} gplayback_cursorpos;

typedef struct {
  gplayback_cursorpos cursor;
  gplayback_slice src;
} gplayback_vm_op_insert_word_after;

typedef struct {
  size_t line;
  gplayback_slice src;
} gplayback_vm_op_insert_row_after;

typedef struct {
  gplayback_cursorpos cursor;
  size_t len;
} gplayback_vm_op_delete_word;

typedef struct {
  size_t line;
} gplayback_vm_op_delete_row;

typedef struct {
  size_t from;
  size_t to;
} gplayback_vm_op_move_row;

typedef struct {
} gplayback_motion;

// Dynamic array types
typedef struct {
  gplayback_vm_operation *items;
  size_t count;
  size_t capacity;
} gplayback_vm_operations;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} gplayback_indices;

typedef struct {
  gplayback_line *items;
  size_t count;
  size_t capacity;
} gplayback_lines;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} gplayback_word_matches;
