#ifndef ASSERT_H
#define ASSERT_H

#include "error.h"
#include "stdio.h"

#define assert(condition, ...)                                                 \
  if (!(condition)) {                                                          \
    char out[4096];                                                            \
    int offset =                                                               \
        snprintf(out, 4096, "ASSERTION FAILED: %s:%d: ", __FILE__, __LINE__);  \
    snprintf(out + offset, 4096 - offset, __VA_ARGS__);                        \
    error_f(out);                                                              \
  }

#endif /* ASSERT_H */
