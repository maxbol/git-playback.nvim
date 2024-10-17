#include "motions.c"

int main() {
  gplayback_slice lhs = strslice("Hello, world!\nBar\nThis is a test.\n");
  gplayback_slice rhs = strslice("Hello, world!\nThis is not a test.\n");

  gplayback_diff diff[] = {
      {1, 1, 14, 24, 14, 20},
  };

  generate_operations(lhs, rhs, diff, 1);

  return 0;
}
