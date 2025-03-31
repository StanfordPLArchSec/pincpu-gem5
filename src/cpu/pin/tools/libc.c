#include "libc.h"

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const int c1 = s1[i];
    const int c2 = s2[i];
    const int diff = c1 - c2;
    if (diff)
      return diff;
    if (c1 == 0)
      return 0;
  }
  return 0;
}
