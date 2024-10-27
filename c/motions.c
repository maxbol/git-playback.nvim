#include "patch.c"

typedef struct {
  int total_cost;
} gplayback_context;

typedef struct {
  gplayback_diff diff;
  gplayback_cursorpos cursor;
  int modal_state;
  char *keys_pressed;
} gplayback_node;

typedef struct {
  int key_cost;
  int modeshift;
  char *keys;
  int (*get_heuristic)(gplayback_node, gplayback_context);
} gplayback_motion;

typedef struct {
  int cost;
  gplayback_node from_node;
  gplayback_node to_node;
} gplayback_branch;

typedef struct {
  gplayback_branch *items;
  size_t count;
  size_t capacity;
} gplayback_branches;

typedef struct {
  gplayback_diff *items;
  size_t count;
  size_t capacity;
} gplayback_diffs;
