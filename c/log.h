#define ENABLE_DEBUG_LOGGING false

#define dbg_log(...)                                                           \
  if (ENABLE_DEBUG_LOGGING) {                                                  \
    fprintf(stderr, __VA_ARGS__);                                              \
  }
