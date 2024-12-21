#ifndef SHOW_H
#define SHOW_H

#define REPO ".git"
#define GIT_SUCCESS 0

#include "git2.h"
#include "slice.h"

gplayback_slice show_file_at_path(const char *file_path);
gplayback_slice show_file_at_rev(git_repository *repo, const char *file_path,
                                 const char *rev);

#endif // !SHOW_H
