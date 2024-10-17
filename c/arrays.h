#define da_append(xs, x)                                                       \
  do {                                                                         \
    if (xs.count >= xs.capacity) {                                             \
      if (xs.capacity == 0)                                                    \
        xs.capacity = 256;                                                     \
      xs.capacity = xs.capacity * 2;                                           \
      xs.items = realloc(xs.items, xs.capacity * sizeof(*xs.items));           \
    }                                                                          \
    xs.items[xs.count++] = x;                                                  \
  } while (0)

#define da_replace(xs, i, x)                                                   \
  do {                                                                         \
    if (i >= xs.capacity) {                                                    \
      xs.capacity = (i + 1) * 2;                                               \
      xs.items = realloc(xs.items, xs.capacity * sizeof(*xs.items));           \
    }                                                                          \
    xs.items[i] = x;                                                           \
  } while (0)

#define da_free(xs)                                                            \
  do {                                                                         \
    free(xs.items);                                                            \
    xs.items = NULL;                                                           \
    xs.count = 0;                                                              \
    xs.capacity = 0;                                                           \
  } while (0)
