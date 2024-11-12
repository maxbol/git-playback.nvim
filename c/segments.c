#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "segments.h"

gplayback_segment subsegment(gplayback_segment segment, int start, int len) {
  gplayback_segment subsegment = {segment.slice, segment.start + start, len};
  return subsegment;
}

const char *segmentptr(gplayback_segment segment) {
  assert(segment.start + segment.len <= segment.slice->len,
         "Segment end is out of bounds");
  return segment.slice->ptr + segment.start;
}

gplayback_slice strslice(const char *ptr) {
  gplayback_slice slice = {ptr, strlen(ptr)};
  return slice;
}

gplayback_slice subslice(const char *ptr, int offset, int len) {
  gplayback_slice slice = {ptr + offset, len};
  return slice;
}

char *slice_to_buf(gplayback_slice slice) {
  char *buf = malloc(slice.len + 1);
  memcpy(buf, slice.ptr, slice.len);
  buf[slice.len + 1] = '\0';
  return buf;
}
