#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#define error(fmt, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%d (%s): ", __FUNCTION__, __LINE__, __FILE__);     \
      fprintf (stderr, fmt, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      exit (1);                                                               \
    }                                                                         \
  while (0)

#define error_if(expr, fmt, ...)                                              \
  do                                                                          \
    if (expr)                                                                 \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0)

#define printf_return(fmt, ...)                                               \
  do                                                                          \
    {                                                                         \
      printf (fmt, ##__VA_ARGS__);                                            \
      return;                                                                 \
    }                                                                         \
  while (0)

#define printf_goto(label, fmt, ...)                                          \
  do                                                                          \
    {                                                                         \
      printf (fmt, ##__VA_ARGS__);                                            \
      goto label;                                                             \
    }                                                                         \
  while (0)

#endif
