#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

size_t escape_string(const char *string, char escaped[], size_t escaped_size) {
  size_t len = strlen(string);
  if (escaped == NULL) {
    return 0;
  }

  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    if (string[i] == '\n') {
      if (j + 2 >= escaped_size) {
        raw_error_f("Escaped string size exceeds size of buffer, panicing\n");
      }
      escaped[j++] = '\\';
      escaped[j++] = 'n';
    } else if (string[i] == '\t') {
      if (j + 2 >= escaped_size) {
        raw_error_f("Escaped string size exceeds size of buffer, panicing\n");
      }
      escaped[j++] = '\\';
      escaped[j++] = 't';
    } else {
      if (j + 1 >= escaped_size) {
        raw_error_f("Escaped string size exceeds size of buffer, panicing\n");
      }
      escaped[j++] = string[i];
    }
  }
  escaped[j] = '\0';
  return j;
}
