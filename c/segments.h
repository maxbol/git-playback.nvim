#ifndef SEGMENTS_H
#define SEGMENTS_H
#include <stdlib.h>

typedef struct {
  const char *ptr;
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

gplayback_segment subsegment(gplayback_segment segment, int start, int len);
const char *segmentptr(gplayback_segment segment);
gplayback_slice strslice(const char *ptr);
gplayback_slice subslice(char *ptr, int offset, int len);
char *slice_to_buf(gplayback_slice slice);
void free_slice_buf(gplayback_slice slice);
gplayback_slice copy_slice(gplayback_slice slice);

#define slicecmp(a, b, c)                                                      \
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
