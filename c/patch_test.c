#include "constants.h"
#include "diff.h"
#include "patch.h"
#include "slice.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_patch_generate_cutandpastewords(void) {
  gplayback_slice lhs_slice = slice_from_buf("foo bar baz\n");
  gplayback_slice rhs_slice = slice_from_buf("foo baz bar\n");

  gplayback_diff diff = diff_generate(
      lhs_slice, rhs_slice,
      (gplayback_generate_diff_opts){.moveword_min_word_amount = 1,
                                     .moveline_entropy_treshold = 0.5});

  printf("Diff:\n");
  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  gplayback_patch patch = patch_generate(&diff);

  gplayback_vm_operation_entry *entry = patch.first;
  TEST_ASSERT(entry != NULL);

  TEST_ASSERT_EQUAL(GPLAYBACK_OP_CUT_WORDS, entry->item.type);
  TEST_ASSERT_EQUAL(0, entry->item.cursor.line);
  TEST_ASSERT_EQUAL(8, entry->item.cursor.column);

  gplayback_vm_op_cut_words *cut_words = entry->item.data;
  TEST_ASSERT_EQUAL(3, cut_words->char_len);

  entry = entry->next;
  TEST_ASSERT(entry != NULL);

  TEST_ASSERT_EQUAL(GPLAYBACK_OP_PASTE_WORDS, entry->item.type);
  TEST_ASSERT_EQUAL(0, entry->item.cursor.line);
  TEST_ASSERT_EQUAL(4, entry->item.cursor.column);

  entry = entry->next;
  TEST_ASSERT(entry != NULL);

  TEST_ASSERT_EQUAL(GPLAYBACK_OP_INSERT_WORD_BEFORE, entry->item.type);
  TEST_ASSERT_EQUAL(0, entry->item.cursor.line);
  TEST_ASSERT_EQUAL(7, entry->item.cursor.column);

  gplayback_vm_op_insert_word_before *insert_word_before = entry->item.data;
  TEST_ASSERT_EQUAL(1, insert_word_before->src.len);
  TEST_ASSERT_EQUAL(' ', insert_word_before->src.ptr[0]);

  entry = entry->next;
  TEST_ASSERT(entry != NULL);

  TEST_ASSERT_EQUAL(GPLAYBACK_OP_DELETE_WORDS, entry->item.type);
  TEST_ASSERT_EQUAL(0, entry->item.cursor.line);
  TEST_ASSERT_EQUAL(11, entry->item.cursor.column);

  gplayback_vm_op_delete_words *delete_words = entry->item.data;

  TEST_ASSERT_EQUAL(1, delete_words->char_len);

  TEST_ASSERT(entry->next == NULL);
}
