#include <stdlib.h>

typedef struct {
  char *ptr;
  size_t len;
} gplayback_slice;

typedef struct {
  gplayback_slice *slice;
  int start;
  int len;
} gplayback_segment;

typedef struct {
  gplayback_slice *items;
  size_t count;
  size_t capacity;
} gplayback_slices;

typedef struct {
  gplayback_segment *items;
  size_t count;
  size_t capacity;
} gplayback_segments;
