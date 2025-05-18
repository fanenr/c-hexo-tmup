#include "config.h"
#include "mstr.h"
#include "util.h"

#include <locale.h>
#include <time.h>

#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>

#define WATCH_FLAG IN_CLOSE_WRITE

static int inotify;
static mstr_t entries[WATCH_SIZE];

static void add_watch (const char *path);
static void work (struct inotify_event *event);
static size_t get_time (char *buff, size_t size);

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");
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
      size_t n = read (inotify, buff, sizeof (buff));
      error_if (n <= 0, "read from inotify");

      for (char *ptr = buff; ptr < buff + n;
	   ptr += sizeof (struct inotify_event) + event->len)
	{
	  event = (struct inotify_event *) ptr;
	  char *path = mstr_data (entries + event->wd);
	  inotify_add_watch (inotify, path, IN_ONLYDIR);
	  work (event);
	  inotify_add_watch (inotify, path, WATCH_FLAG);
	}
    }
}

static inline void
add_watch (const char *path)
{
  char sub[NAME_SIZE];
  DIR *dir = opendir (path);
  error_if (!dir, "opendir");
  size_t len = strlen (path);

  for (struct dirent *item; (item = readdir (dir));)
    {
      if (item->d_type != DT_DIR)
	continue;

      char *name = item->d_name;
      size_t size = strlen (name);

      if (size == 1 && name[0] == '.')
	{
	  int wfd = inotify_add_watch (inotify, path, WATCH_FLAG);
	  error_if (wfd < 0, "inotify_add_watch");
	  printf ("watching: %s\n", path);

	  mstr_t *mstr = entries + wfd;
	  if (!mstr_assign_byte (mstr, path, len))
	    error ("mstr_assign_byte");

	  continue;
	}

      if (size == 2 && name[0] == '.' && name[1] == '.')
	continue;

      sub[len] = '/';
      memcpy (sub, path, len);
      memcpy (sub + len + 1, name, size + 1);

      add_watch (sub);
    }

  closedir (dir);
}

static inline size_t
get_time (char *dest, size_t size)
{
  time_t raw = time (NULL);
  struct tm *tm = localtime (&raw);
  error_if (tm == NULL, "localtime");
  return strftime (dest, size, "%Y-%m-%d %H:%M:%S", tm);
}

static inline void
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
  char buff[NAME_SIZE];
  size_t space = 0;
  size_t tsize;
  FILE *fs;

  printf ("\nfile: %s/%s\n", data, name);
  if (strcmp (name + size - 3, ".md") != 0)
    printf_return ("    not a markdown file: %s\n", event->name);

  buff[len] = '/';
  memcpy (buff, data, len);
  memcpy (buff + len + 1, name, size + 1);

  /* delay */
  struct timespec ts = { .tv_nsec = WRITE_DELAY };
  nanosleep (&ts, NULL);

  /* open */
  if (!(fs = fopen (buff, "r+")))
    printf_return ("    open failed!\n");

  ssize_t llen;	       /* line length */
  static char *line;   /* getline buffer data */
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

  if ((tsize = get_time (buff, sizeof (buff))) == 0)
    printf_goto (clean_fs, "    get time failed!\n");
  if (fseek (fs, 9L - llen, SEEK_CUR) != 0)
    printf_goto (clean_fs, "    fseek failed!\n");
  printf ("    new time: %s\n", buff);

  for (int i = 9, ch; i < llen; i++, space++)
    if (!(ch = line[i]) || ch == '\n' || space >= tsize)
      break;

  if (space < tsize)
    printf_goto (clean_fs, "    there's not enough sapce to overwrite\n");

  /* overwrite */
  if (fwrite (buff, 1, tsize, fs) != tsize)
    printf_goto (clean_fs, "    fwrite failed!\n");

  printf ("    updated successfully!\n");

clean_fs:
  fclose (fs);
}
