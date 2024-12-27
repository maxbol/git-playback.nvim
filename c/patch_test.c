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

void test_patch_insert_word_before(void) {
  const char *lhs = "\n"
                    "\n"
                    "\n"
                    "Yoyo and Hello, my beautiful world!\n"
                    "Bar biez\n"
                    "This is not a test.\n"
                    "\n"
                    "\n"
                    "  This is an indented line\n"
                    "\n"
                    "  Foo\n"
                    "This is another line.\n"
                    "\n"
                    "\n"
                    "And this is another one\n";

  const char *rhs = "\n"
                    "\n"
                    "\n"
                    "Yoyo and Hello, my cool beautiful world!\n"
                    "Bar biez\n"
                    "This is not a test.\n"
                    "\n"
                    "\n"
                    "  This is an indented line\n"
                    "\n"
                    "  Foo\n"
                    "This is another line.\n"
                    "\n"
                    "\n"
                    "And this is another one\n";

  gplayback_slice lhs_slice = slice_from_buf(lhs);
  gplayback_slice rhs_slice = slice_from_buf(rhs);

  gplayback_diff diff = diff_generate(lhs_slice, rhs_slice,
                                      (gplayback_generate_diff_opts){
                                          .moveword_min_word_amount = 3,
                                          .moveline_entropy_treshold = 0.5,
                                      });

  printf("Diff:\n");
  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  gplayback_patch patch = patch_generate(&diff);

  char *patch_debug_out = patch_debug(&patch);
  printf("Patch:\n%s\n", patch_debug_out);
  free(patch_debug_out);

  gplayback_vm_operation_entry *entry = patch.first;

  TEST_ASSERT(entry != NULL);

  TEST_ASSERT_EQUAL(GPLAYBACK_OP_INSERT_WORD_BEFORE, entry->item.type);
  TEST_ASSERT_EQUAL(3, entry->item.cursor.line);
  TEST_ASSERT_EQUAL(19, entry->item.cursor.column);

  gplayback_vm_op_insert_word_before *insert_word_before = entry->item.data;

  TEST_ASSERT_EQUAL(5, insert_word_before->src.len);
  TEST_ASSERT_EQUAL('c', insert_word_before->src.ptr[0]);
  TEST_ASSERT_EQUAL('o', insert_word_before->src.ptr[1]);
  TEST_ASSERT_EQUAL('o', insert_word_before->src.ptr[2]);
  TEST_ASSERT_EQUAL('l', insert_word_before->src.ptr[3]);
  TEST_ASSERT_EQUAL(' ', insert_word_before->src.ptr[4]);

  TEST_ASSERT(entry->next == NULL);
}
