#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define error(fmt, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%d (%s): ", __FUNCTION__, __LINE__, __FILE__);     \
      fprintf (stderr, fmt, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      __builtin_trap ();                                                      \
    }                                                                         \
  while (0)

#define error_if(expr, fmt, ...)                                              \
  do                                                                          \
    if (expr)                                                                 \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0)

#endif
