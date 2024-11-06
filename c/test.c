#include "patch.c"

int main() {
  // fooo
  gplayback_slice lhs =
      strslice("Hello, world!\nBar\nbie\nYoyo\nGogo\nThis is a test.\nFubu\n");
  gplayback_slice rhs =
      strslice("Hello, world!\nThis is not a test.\nBar bie doll\nYoyo\n");

  gplayback_diff diff = generate_diff(lhs, rhs);

  printf("Original diff:\n");
  debug_diff(diff);

  gplayback_patch patch = generate_patch(diff);

  printf("Diff after patch generation:\n");
  debug_diff(diff);

  debug_patch(patch);

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
