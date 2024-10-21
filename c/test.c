#include "motions.c"

// Basic text editor heuristics
int move_heuristic(gplayback_node node, gplayback_context context,
                   void *closure) {
  return 1;
}

int backspace_heuristic(gplayback_node node, gplayback_context context,
                        void *closure) {
  return 1;
}

typedef struct {
  char key;
} bte_closure_typechar;
int typechar_heuristic(gplayback_node node, gplayback_context context,
                       void *closure) {
  bte_closure_typechar *c = closure;
  gplayback_text *lhs = &node.diff.lhs;
  gplayback_text *rhs = &node.diff.rhs;

  if (lhs->words_len == rhs->words_len) {
    gplayback_word_list_entry *lhs_word = lhs->words.first;
    gplayback_word_list_entry *rhs_word = rhs->words.first;
    int i = 0;
    do {
      if (lhs_word->item.len != rhs_word->item.len) {
        return 0;
      }
    } while ((++i) > lhs->words_len && (lhs_word = lhs_word->next) &&
             (rhs_word = rhs_word->next));
  }
  return 1;
}

int main() {
  gplayback_slice lhs =
      strslice("Hello, world!\nBar\nbie\nYoyo\nGogo\nThis is a test.\nFubu\n");
  gplayback_slice rhs =
      strslice("Hello, world!\nThis is not a test.\nBar bie\nYoyo\n");

  gplayback_diff diff = generate_diff(lhs, rhs);
  gplayback_patch patch = generate_patch(diff);

  printf("Operations generated:\n");
  gplayback_vm_operation_entry *entry = patch.first;
  while (entry != NULL) {
    printf(" + Operation type: %d\n", entry->item.type);
    printf(" + Cursor: %zu:%zu -> %zu:%zu\n", entry->item.cursor.line,
           entry->item.cursor.column, entry->item.cursor_after.line,
           entry->item.cursor_after.column);

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
    }
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
 * Bar bie
 * Yoyo
 */
