#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "assert.h"
#include "segments.h"

#define REPO ".git"
#define GIT_SUCCESS 0

gplayback_slice show_file_at_path(const char *file_path) {
  if (access(file_path, F_OK) == -1) {
    return strslice("");
  }

  FILE *file = fopen(file_path, "r");
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);

  rewind(file);

  char *txt_ptr = malloc(sizeof(char) * (file_size + 1));
  memset(txt_ptr, 0, file_size + 1);
  fread(txt_ptr, file_size, 1, file);

  gplayback_slice txt = strslice(txt_ptr);

  fclose(file);

  return txt;
}

gplayback_slice show_file_at_rev(const char *file_path, const char *rev) {
  git_repository *repo = NULL;
  git_object *obj = NULL;
  git_tree *tree = NULL;
  git_blob *blob = NULL;
  git_commit *commit = NULL;
  git_tree_entry *entry = NULL;
  int success = 0;

  // Open the repository
  success = git_repository_open(&repo, REPO);
  assert(success == GIT_SUCCESS, "Could not open repository %s\n",
         git_error_last()->message);

  // Resolve the revision
  success = git_revparse_single(&obj, repo, rev);
  assert(success == GIT_SUCCESS, "Could not resolve revision: %s",
         git_error_last()->message);

  // Get the tree from the commit
  if (git_object_type(obj) == GIT_OBJ_COMMIT) {
    commit = (git_commit *)obj;
    success = git_commit_tree(&tree, commit);
    assert(success == GIT_SUCCESS, "Could not get commit tree: %s",
           git_error_last()->message);
  } else if (git_object_type(obj) == GIT_OBJ_TREE) {
    tree = (git_tree *)obj;
  } else {
    error("Object is not a commit or tree\n");
  }

  // Get the tree entry
  success = git_tree_entry_bypath(&entry, tree, file_path);
  if (success == GIT_ENOTFOUND) {
    // File not in tree at this revision, return empty string
    git_object_free(obj);
    git_tree_free(tree);
    git_commit_free(commit);
    git_repository_free(repo);

    return strslice("");
  }
  assert(success == GIT_SUCCESS, "Could not get tree entry: %s",
         git_error_last()->message);

  // Get the blob from the tree entry
  success = git_tree_entry_to_object(&obj, repo, entry);
  assert(success == GIT_SUCCESS, "Could not get tree entry object: %s",
         git_error_last()->message);

  blob = (git_blob *)obj;

  size_t blob_raw_size = git_blob_rawsize(blob);
  char *txt_ptr = malloc(sizeof(char) * (blob_raw_size + 1));
  memset(txt_ptr, 0, blob_raw_size + 1);
  strncpy(txt_ptr, git_blob_rawcontent(blob), blob_raw_size);
  gplayback_slice txt = strslice(txt_ptr);

  // Cleanup
  git_object_free(obj);
  git_tree_entry_free(entry);
  git_tree_free(tree);
  git_commit_free(commit);
  git_repository_free(repo);

  return txt;
}
