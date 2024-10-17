#include <stdio.h>
#include <string.h>

#include "segments.h"

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
