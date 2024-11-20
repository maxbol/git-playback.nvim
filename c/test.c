#include <git2.h>
#include <stdio.h>

#include "diff.h"
#include "patch.h"
#include "segments.h"
#include "show.h"

int main() {
  git_libgit2_init();

  // fooo
  gplayback_slice lhs = show_file_at_rev("c/assert.c", "HEAD");
  gplayback_slice rhs = show_file_at_path("c/assert.c");

  printf("LHS\n===\n%.*s\n===\n\n", (int)lhs.len, lhs.ptr);
  printf("RHS\n===\n%.*s\n===\n\n", (int)rhs.len, rhs.ptr);

  gplayback_diff diff = generate_diff(lhs, rhs);

  printf("Original diff:\n");
  char *original_diff_out = debug_diff(diff);
  printf("%s\n", original_diff_out);
  free(original_diff_out);

  gplayback_patch patch = generate_patch(&diff);

  printf("Diff after patch generation:\n");
  /*char *patched_diff_out = debug_diff(diff);*/
  /*printf("%s\n", patched_diff_out);*/
  /*free(patched_diff_out);*/

  if (patch.first == NULL) {
    printf("Patch is empty\n");
  } else {
    printf("Patch is NOT empty\n");
  }

  char *patch_out = debug_patch(patch);
  printf("%s\n", patch_out);
  free(patch_out);

  free_patch(patch);
  free_diff(diff);

  free_slice_buf(lhs);
  free_slice_buf(rhs);

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
