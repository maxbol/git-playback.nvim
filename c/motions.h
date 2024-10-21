#include "segments.c"
#include <stdio.h>

#define GPLAYBACK_OP_INSERT_WORD_AFTER 1
#define GPLAYBACK_OP_INSERT_ROW_AFTER 2
#define GPLAYBACK_OP_DELETE_WORDS 3
#define GPLAYBACK_OP_DELETE_ROWS 4
#define GPLAYBACK_OP_MOVE_ROWS 5
#define GPLAYBACK_OP_CONCAT_ROWS 6
#define GPLAYBACK_OP_SPLIT_ROWS 7

#define GPLAYBACK_REPLACE_SIM_ENTROPY 1.5
#define GPLAYBACK_REPLACE_SIM_TRESHOLD 0.25

typedef struct gplayback_word {
  char *ptr;
  struct gplayback_word_list_entry *match;
  int line_idx;
  int len;
} gplayback_word;

typedef struct gplayback_word_list_entry {
  gplayback_word item;
  struct gplayback_word_list_entry *next;
  struct gplayback_word_list_entry *prev;
} gplayback_word_list_entry;

typedef struct {
  gplayback_word_list_entry *first;
} gplayback_word_list;

typedef struct gplayback_line {
  bool dirty;
  size_t word_len;
  size_t char_len;
  size_t line_num;
  gplayback_word_list_entry *words;
  struct gplayback_line *match;
} gplayback_line;

/*typedef struct {*/
/*  int from_file_line_start;*/
/*  int to_file_line_start;*/
/*  size_t from_file_char_start;*/
/*  size_t from_file_char_len;*/
/*  size_t to_file_char_start;*/
/*  size_t to_file_char_len;*/
/*} gplayback_diff;*/

typedef struct {
  size_t line;
  size_t column;
  /*size_t chr;*/
} gplayback_cursorpos;

typedef struct {
  int type;
  void *data;
  gplayback_cursorpos cursor;
  gplayback_cursorpos cursor_after;
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
  size_t char_len;
} gplayback_vm_op_delete_words;

typedef struct {
  size_t no_of_lines;
} gplayback_vm_op_delete_rows;

typedef struct {
  size_t no_of_lines;
  int move_amount;
} gplayback_vm_op_move_rows;

// Dynamic array types
typedef struct {
  gplayback_vm_operation_entry *first;
} gplayback_patch;

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

typedef struct {
  gplayback_word_list words;
  gplayback_lines lines;
  int words_len;
} gplayback_text;

typedef struct {
  gplayback_text lhs;
  gplayback_text rhs;
} gplayback_diff;

typedef struct {
  gplayback_diff diff;
  gplayback_cursorpos cursor;
  int modal_state;
  char *keys_pressed;
} gplayback_node;

typedef struct {
  int total_cost;
} gplayback_context;

typedef struct {
  int key_cost;
  int modeshift;
  char *keys;
  int (*get_heuristic)(gplayback_node, gplayback_context);
} gplayback_motion;

typedef struct {
  int cost;
  gplayback_node from_node;
  gplayback_node to_node;
} gplayback_branch;

typedef struct {
  gplayback_branch *items;
  size_t count;
  size_t capacity;
} gplayback_branches;

typedef struct {
  gplayback_diff *items;
  size_t count;
  size_t capacity;
} gplayback_diffs;
