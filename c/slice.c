#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "slice.h"

gplayback_slice slice_copy(gplayback_slice slice) {
  return slice_from_buf(slice_to_buf(slice));
}

void slice_free_buf(gplayback_slice slice) { free((void *)slice.ptr); }
gplayback_slice slice_from_buf(const char *ptr) {
  gplayback_slice slice = {.ptr = ptr, .len = strlen(ptr)};
  return slice;
}

gplayback_slice slice_subslice(const char *ptr, int offset, int len) {
  gplayback_slice slice = {.ptr = ptr + offset, .len = len};
  return slice;
}

char *slice_to_buf(gplayback_slice slice) {
  char *buf = malloc(slice.len + 1);
  memset(buf, 0, slice.len + 1);
  memcpy(buf, slice.ptr, slice.len);
  buf[slice.len + 1] = '\0';
  return buf;
}
