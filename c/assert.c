#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void assert(bool condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "Assertion failed: %s\n", message);
    exit(1);
  }
}
