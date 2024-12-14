#ifndef SEGMENTS_H
#define SEGMENTS_H
#include <stdlib.h>

typedef struct {
  const char *ptr;
  size_t len;
} gplayback_slice;

typedef struct {
  gplayback_slice *items;
  size_t count;
  size_t capacity;
} gplayback_slices;

char *slice_to_buf(gplayback_slice slice);
gplayback_slice slice_copy(gplayback_slice slice);
gplayback_slice slice_from_buf(const char *ptr);
gplayback_slice slice_subslice(const char *ptr, int offset, int len);
void slice_free_buf(gplayback_slice slice);

#define slice_cmp(a, b, c)                                                     \
  do {                                                                         \
    if (a.len != b.len) {                                                      \
      *c = false;                                                              \
      break;                                                                   \
    }                                                                          \
    for (size_t i = 0; i < a.len; i++) {                                       \
      if (a.ptr[i] != b.ptr[i]) {                                              \
        *c = false;                                                            \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    *c = true;                                                                 \
  } while (0)

#endif // !SEGMENTS_H
