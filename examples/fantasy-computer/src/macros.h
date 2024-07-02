#include <stdio.h>

#define DEBUG_MESSAGE(level, ...)                                              \
  do {                                                                         \
    fprintf(stderr, "%s:%d: [%s] ", __FILE__, __LINE__, level);                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)

#define ERROR(...) DEBUG_MESSAGE("error", __VA_ARGS__)
#define LOG(...) DEBUG_MESSAGE("log", __VA_ARGS__)