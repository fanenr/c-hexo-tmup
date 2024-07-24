#include "config.h"
#include "mstr.h"
#include "util.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>

#define WATCH_FLAG IN_CLOSE_WRITE

static int inotify;
static mstr_t entries[] = { [0 ... WATCH_SIZE - 1] = MSTR_INIT };

static void add_watch (const char *dpath);
static void work (struct inotify_event *event);
static size_t get_time (char *dest, size_t size);

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "en_US.UTF-8");
  error_if (argc < 2, "Usage: %s <hexo-posts-dir>\n", argv[0]);

  char *dir = argv[1];
  char buff[BUFF_SIZE];

  size_t dlen = strlen (dir);
  if (dir[dlen - 1] == '/')
    dir[dlen - 1] = 0;

  inotify = inotify_init ();
  error_if (inotify < 0, "inotify_init");

  add_watch (dir);

  for (struct inotify_event *event;;)
    {
      size_t n = read (inotify, buff, BUFF_SIZE);
      error_if (n <= 0, "read from inotify");

      for (char *ptr = buff; ptr < buff + n;
           ptr += sizeof (struct inotify_event) + event->len)
        {
          event = (struct inotify_event *)ptr;
          char *path = mstr_data (entries + event->wd);
          inotify_add_watch (inotify, path, IN_ONLYDIR);
          work (event);
          inotify_add_watch (inotify, path, WATCH_FLAG);
        }
    }

  for (int i = 0; i < WATCH_SIZE; i++)
    mstr_free (entries + i);
}

static void
add_watch (const char *dpath)
{
  char sub[NAME_SIZE];
  DIR *dir = opendir (dpath);
  error_if (!dir, "opendir");
  size_t len = strlen (dpath);

  for (struct dirent *item; (item = readdir (dir));)
    {
      if (item->d_type != DT_DIR)
        continue;

      char *name = item->d_name;
      size_t size = strlen (name);

      if (size == 1 && name[0] == '.')
        {
          int wfd = inotify_add_watch (inotify, dpath, WATCH_FLAG);
          error_if (wfd < 0, "inotify_add_watch");
          printf ("watching: %s\n", dpath);

          mstr_t *mstr = entries + wfd;
          if (!mstr_assign_byte (mstr, dpath, len))
            error ("mstr_assign_byte");

          continue;
        }

      if (size == 2 && name[0] == '.' && name[1] == '.')
        continue;

      sub[len] = '/';
      memcpy (sub, dpath, len);
      memcpy (sub + len + 1, name, size + 1);

      add_watch (sub);
    }

  closedir (dir);
}

static size_t
get_time (char *dest, size_t size)
{
  time_t raw = time (NULL);
  struct tm *tm = localtime (&raw);
  error_if (tm == NULL, "localtime");
  return strftime (dest, size, "%Y-%m-%d %H:%M:%S", tm);
}

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

static void
work (struct inotify_event *event)
{
  char *name = event->name;

  /* filter */
  if (name[0] == '.')
    return;

  mstr_t *mstr = entries + event->wd;
  char *data = mstr_data (mstr);
  size_t len = mstr_len (mstr);
  size_t size = strlen (name);
  char path[NAME_SIZE];
  char time[TIME_SIZE];
  size_t space = 0;
  size_t tsize;
  FILE *fs;

  printf ("\nfile: %s/%s\n", data, name);
  if (strcmp (name + size - 3, ".md") != 0)
    printf_return ("    not a markdown file: %s\n", event->name);

  path[len] = '/';
  memcpy (path, data, len);
  memcpy (path + len + 1, name, size + 1);

  /* delay */
  struct timespec ts = { .tv_nsec = WRITE_DELAY };
  nanosleep (&ts, NULL);

  /* open */
  if (!(fs = fopen (path, "r+")))
    printf_return ("    open failed!\n");

  ssize_t llen;        /* line length */
  static char *line;   /* getline buffer data*/
  static size_t lsize; /* getline buffer size */

  for (int front = 0;;)
    {
      if ((llen = getline (&line, &lsize, fs)) == -1)
        printf_goto (clean_fs, "    getline failed!\n");

      if (strncmp (line, "---", 3) == 0)
        front++;

      if (front > 1)
        printf_goto (clean_fs, "    didn't find updated field\n");

      if (!strncmp (line, "updated: ", 9))
        break;
    }

  if ((tsize = get_time (time, sizeof (time))) == 0)
    printf_goto (clean_fs, "    get time failed!\n");
  if (fseek (fs, 9L - llen, SEEK_CUR) != 0)
    printf_goto (clean_fs, "    fseek failed!\n");
  printf ("    new time: %s\n", time);

  for (int i = 9, ch; i < llen; i++, space++)
    if (!(ch = line[i]) || ch == '\n' || space >= tsize)
      break;

  if (space < tsize)
    printf_goto (clean_fs, "    there's not enough sapce to overwrite\n");

  /* overwrite */
  if (fwrite (time, sizeof (char), tsize, fs) != tsize)
    printf_goto (clean_fs, "    fwrite failed!\n");

  printf ("    updated successfully!\n");

clean_fs:
  fclose (fs);
}
