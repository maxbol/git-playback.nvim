#include <git2.h>
#include <stdio.h>

#include "diff.h"
#include "patch.h"
#include "segments.h"
#include "show.h"

int main() {
  git_libgit2_init();

  // fooo
  gplayback_slice lhs = show_file_at_rev("test.txt", "HEAD");
  gplayback_slice rhs = show_file_at_path("test.txt");
  /*gplayback_slice lhs = strslice(*/
  /*    "Hello, world!\nBar\nbie\nYoyo\nGogo\nThis is a test.\nFubu\n\n");*/
  /*gplayback_slice rhs =*/
  /*    strslice("Hello, world!\nThis is not a test.\nBar bie doll\nYoyo\n");*/

  printf("Hello, world!\n");

  gplayback_diff diff = generate_diff(lhs, rhs);

  printf("Original diff:\n");
  char out[4096];
  debug_diff(diff, out, 4096);
  printf("%s\n", out);

  gplayback_patch patch = generate_patch(diff);

  printf("Diff after patch generation:\n");
  debug_diff(diff, out, 4096);
  printf("%s\n", out);

  debug_patch(patch, out, 4096);
  printf("%s\n", out);

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
