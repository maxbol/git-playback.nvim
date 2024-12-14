#include <git2.h>
#include <stdio.h>

#include "diff.h"
#include "patch.h"
#include "show.h"
#include "slice.h"
#include "writestr.h"

int main() {
  git_libgit2_init();

  // fooo
  gplayback_slice lhs = show_file_at_rev("test.txt", "HEAD~2");
  gplayback_slice rhs = show_file_at_path("test.txt");
  /*gplayback_slice rhs = show_file_at_rev("test.txt", "HEAD");*/

  printf("LHS\n===\n%.*s\n===\n\n", (int)lhs.len, lhs.ptr);
  printf("RHS\n===\n%.*s\n===\n\n", (int)rhs.len, rhs.ptr);

  gplayback_diff diff = diff_generate(
      lhs, rhs,
      (gplayback_generate_diff_opts){.moveline_entropy_treshold = 0.5,
                                     .moveword_min_word_amount = 1});

  printf("Original diff:\n");
  char *original_diff_out = diff_debug(&diff);
  printf("%s\n", original_diff_out);
  free(original_diff_out);

  gplayback_patch patch = patch_generate(&diff);

  printf("Diff after patch generation:\n");
  char *patched_diff_out = diff_debug(&patch.diff);
  printf("%s\n", patched_diff_out);
  free(patched_diff_out);

  if (patch.first == NULL) {
    printf("Patch is empty\n");
  } else {
    printf("Patch is NOT empty\n");
  }

  char *patch_out = patch_debug(&patch);
  printf("%s\n", patch_out);
  free(patch_out);

  patch_free(&patch);
  diff_free(&diff);

  slice_free_buf(lhs);
  slice_free_buf(rhs);

  printf("Done\n");

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
