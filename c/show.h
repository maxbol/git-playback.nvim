#ifndef SHOW_H
#define SHOW_H

#define REPO ".git"
#define GIT_SUCCESS 0

#include "segments.h"

gplayback_slice show_file_at_path(const char *file_path);
gplayback_slice show_file_at_rev(const char *file_path, const char *rev);

#endif // !SHOW_H
