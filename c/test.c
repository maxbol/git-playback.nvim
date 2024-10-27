#include "patch.c"

// Basic text editor heuristics
typedef struct {
  char key;
} bte_closure_typechar;
int main() {
  gplayback_slice lhs =
      strslice("Hello, world!\nBar bie\nYoyo\nThis is a test.\n");
  gplayback_slice rhs =
      strslice("Hello, world!\nThis is a test.\nBar bie\nYoyo\n");

  gplayback_diff diff = generate_diff(lhs, rhs);

  printf("Original diff:\n");
  debug_diff(diff);

  gplayback_patch patch = generate_patch(diff);

  printf("Diff after patch generation:\n");
  debug_diff(diff);

  printf("Operations generated:\n");
  gplayback_vm_operation_entry *entry = patch.first;
  while (entry != NULL) {
    switch (entry->item.type) {
    case GPLAYBACK_OP_INSERT_WORD_AFTER: {
      gplayback_vm_op_insert_word_after *data = entry->item.data;
      printf(" >> Insert word after [src=\"%.*s\"]\n", (int)data->src.len,
             data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_INSERT_ROW_AFTER: {
      gplayback_vm_op_insert_row_after *data = entry->item.data;
      printf(" >> Insert row after [src=\"%.*s\"]\n", (int)data->src.len,
             data->src.ptr);
      break;
    }
    case GPLAYBACK_OP_MOVE_ROWS: {
      gplayback_vm_op_move_rows *data = entry->item.data;
      printf(" >> Move rows [no_of_lines=%zu, move_amount=%d]\n",
             data->no_of_lines, data->move_amount);
      break;
    }
    case GPLAYBACK_OP_DELETE_ROWS: {
      gplayback_vm_op_delete_rows *data = entry->item.data;
      printf(" >> Delete rows [len=%zu]\n", data->no_of_lines);
      break;
    }
    case GPLAYBACK_OP_DELETE_WORDS: {
      gplayback_vm_op_delete_words *data = entry->item.data;
      printf(" >> Delete words [len=%zu]\n", data->char_len);
      break;
    }
    case GPLAYBACK_OP_CONCAT_ROWS: {
      printf(" >> Concat rows\n");
      break;
    }
    case GPLAYBACK_OP_SPLIT_ROWS: {
      printf(" >> Split rows\n");
      break;
    }
    }
    printf(" + Cursor: %d:%d -> %d:%d\n", entry->item.cursor.line,
           entry->item.cursor.column, entry->item.cursor_after.line,
           entry->item.cursor_after.column);

    printf("\n");

    entry = entry->next;
  }

  return 0;
}

/*
 * Hello, world!
 * Bar
 * bie
 * Yoyo
 * Gogo
 * This is a test.
 * Fubu
 */

/*
 * Hello, world!
 * This is not a test.
 * Bar bie doll
 * Yoyo
 */
