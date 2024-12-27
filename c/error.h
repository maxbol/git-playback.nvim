#ifndef ERROR_H
#define ERROR_H
#include <stdio.h>

void error_f(const char *message);
void raw_error_f(const char *message);
void pure_error_f(const char *message);

#define error(...)                                                             \
  {                                                                            \
    char out[4096];                                                            \
    int offset = snprintf(out, 4096, "ERROR: %s:%d: ", __FILE__, __LINE__);    \
    snprintf(out + offset, 4096 - offset, __VA_ARGS__);                        \
    error_f(out);                                                              \
  }

#endif /* ERROR_H */
