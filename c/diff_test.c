#include "constants.h"
#include "diff.h"
#include "slice.h"
#include "unity.h"
#include "words.h"

void setUp(void) {}

void tearDown(void) {}

void test_diff_movelines_minimal(void) {
  gplayback_slice lhs_slice = slice_from_buf("foo\nbar\nbaz\n");
  gplayback_slice rhs_slice = slice_from_buf("foo\nbaz\nbar\n");

  gplayback_diff diff = diff_generate(
      lhs_slice, rhs_slice,
      (gplayback_generate_diff_opts){.moveword_min_word_amount = 1,
                                     .moveline_entropy_treshold = 0.5});

  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  unsigned int cursor = diff.rhs.words.first;
  unsigned int i = 0;

  while (cursor != 0) {
    gplayback_diff_moveline moveline = diff.movelines[cursor];

    if (i > 5) {
      TEST_ASSERT(false);
    } else if (i == 4) {
      TEST_ASSERT_EQUAL(1, moveline.lines_amount);
      TEST_ASSERT_EQUAL(5, moveline.lhs_anchor);
    } else {
      TEST_ASSERT_EQUAL(0, moveline.lhs_anchor);
      TEST_ASSERT_EQUAL(0, moveline.lines_amount);
    }

    i++;
    cursor = words_next(&diff.rhs.words, cursor);
  }
}

void test_diff_movelines_multiline(void) {
  gplayback_slice lhs_slice = slice_from_buf("foo\nbar\nbaz\nline2\n");
  gplayback_slice rhs_slice = slice_from_buf("foo\nbaz\nline2\nbar\n");

  gplayback_diff diff = diff_generate(
      lhs_slice, rhs_slice,
      (gplayback_generate_diff_opts){.moveword_min_word_amount = 1,
                                     .moveline_entropy_treshold = 0.5});

  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  unsigned int cursor = diff.rhs.words.first;
  unsigned int i = 0;

  while (cursor != 0) {
    gplayback_diff_moveline moveline = diff.movelines[cursor];

    if (i > 7) {
      TEST_ASSERT(false);
    } else if (i == 4) {
      TEST_ASSERT_EQUAL(2, moveline.lines_amount);
      TEST_ASSERT_EQUAL(5, moveline.lhs_anchor);
    } else {
      TEST_ASSERT_EQUAL(0, moveline.lhs_anchor);
      TEST_ASSERT_EQUAL(0, moveline.lines_amount);
    }

    i++;
    cursor = words_next(&diff.rhs.words, cursor);
  }
}

void test_diff_movelines_halfmatched_lines(void) {
  gplayback_slice lhs_slice = slice_from_buf("foo\nbar\nbaz 1\n");
  gplayback_slice rhs_slice = slice_from_buf("foo\nbaz 2\nbar\n");

  gplayback_diff diff = diff_generate(
      lhs_slice, rhs_slice,
      (gplayback_generate_diff_opts){.moveword_min_word_amount = 1,
                                     .moveline_entropy_treshold = 0.9});

  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  unsigned int cursor = diff.rhs.words.first;
  unsigned int i = 0;

  while (cursor != 0) {
    gplayback_diff_moveline moveline = diff.movelines[cursor];

    if (i > 7) {
      TEST_ASSERT(false);
    } else if (i == 5) {
      TEST_ASSERT_EQUAL(1, moveline.lines_amount);
      TEST_ASSERT_EQUAL(5, moveline.lhs_anchor);
    } else {
      TEST_ASSERT_EQUAL(0, moveline.lhs_anchor);
      TEST_ASSERT_EQUAL(0, moveline.lines_amount);
    }

    i++;
    cursor = words_next(&diff.rhs.words, cursor);
  }
}

/* Make sure that line matches are evenly spread out and that the first LHS line
 * doesn't eat all the matches */
void test_diff_line_matches(void) {
  gplayback_slice lhs_slice = slice_from_buf("foo bar\nfoo bar\nfoo bar\n");
  gplayback_slice rhs_slice = slice_from_buf("foo bar\nfoo bar\nfoo bar\n");

  gplayback_diff diff = diff_generate(
      lhs_slice, rhs_slice,
      (gplayback_generate_diff_opts){.moveword_min_word_amount = 1,
                                     .moveline_entropy_treshold = 0.5});

  char *debug_out = diff_debug(&diff);
  printf("%s\n", debug_out);
  free(debug_out);

  for (int i = 1; i < diff.lhs.words.entries.head; i++) {
    gplayback_word_list_entry entry = words_get_entry(&diff.lhs.words, i);
    TEST_ASSERT_EQUAL(i, entry.item.match);
    TEST_ASSERT_EQUAL(0, diff.movelines[i].lhs_anchor);
    TEST_ASSERT_EQUAL(0, diff.movewords[i].lhs_start);
  }
}
