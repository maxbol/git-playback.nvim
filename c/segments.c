#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

gplayback_segment subsegment(gplayback_segment segment, int start, int len) {
  gplayback_segment subsegment = {segment.slice, segment.start + start, len};
  return subsegment;
}

char *segmentptr(gplayback_segment segment) {
  if (segment.start > segment.slice->len) {
    fprintf(stderr, "Panic: Segment start is out of bounds\n");
    exit(1);
  }
  return segment.slice->ptr + segment.start;
}

gplayback_slice strslice(char *ptr) {
  gplayback_slice slice = {ptr, strlen(ptr)};
  return slice;
}

gplayback_slice subslice(char *ptr, int offset, int len) {
  gplayback_slice slice = {ptr + offset, len};
  return slice;
}

char *slice_to_buf(gplayback_slice slice) {
  char *buf = malloc(slice.len + 1);
  memcpy(buf, slice.ptr, slice.len);
  buf[slice.len] = '\0';
  return buf;
}

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
