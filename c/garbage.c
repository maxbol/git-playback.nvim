#include <stdbool.h>
#include <stdlib.h>

#include "arrays.h"
#include "motions.h"

gplayback_segments
slice_text_by_newline_positions(gplayback_slice *text_slice, int cursor,
                                size_t text_limit,
                                gplayback_indices text_newlines) {
  gplayback_segments from_segments = {0};

  do {
    size_t from_line_limit = text_limit;
    for (int i = 0; i < text_newlines.count; i++) {
      size_t nl = text_newlines.items[i];
      if (nl >= cursor && nl < text_limit) {
        from_line_limit = nl + 1;
        break;
      }
    }

    gplayback_segment segment = {text_slice, cursor, from_line_limit - cursor};

    da_append(from_segments, segment);

    cursor = from_line_limit;
  } while (cursor < text_limit);

  return from_segments;
}

gplayback_indices text_newline_indices(gplayback_slice text) {
  gplayback_indices newlines = {0};
  for (size_t i = 0; i < text.len; i++) {
    if (text.ptr[i] == '\n') {
      da_append(newlines, i);
    }
  }
  return newlines;
}

gplayback_branches expand_node(gplayback_node node, gplayback_motion motions[],
                               size_t motions_len, gplayback_context context) {
  gplayback_branches branches = {0};

  for (int i = 0; i < motions_len; i++) {
    gplayback_motion motion = motions[i];
    if (motion.get_heuristic == NULL) {
      exit(1);
    }

    int heuristic_cost = motion.get_heuristic(node, context);

    if (heuristic_cost == -1) {
      // Infinite heuristic cost, skip this motion
      continue;
    }

    int cost = context.total_cost + (motion.key_cost * heuristic_cost);

    gplayback_branch branch = {cost, node, node};
    da_append(branches, branch);
  }

  return branches;
}

int write_operations(gplayback_diff diff, gplayback_diff *out) {}
int write_permutations(gplayback_diff operation, gplayback_diff *out) {}
