#include "main.h"
#include <dirent.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>

static void add_watch (const char *dpath);
static void get_time (char *dest, size_t size);
static void work (struct inotify_event *event);

int inotify;
char *entries[WATCH_SIZE];

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "en_US.UTF-8");
  CHECK (argc == 2, "must specify a listening path");

  char *path = argv[1];
  size_t plen = strlen (path);
  (path[plen - 1] == '/') ? path[plen - 1] = '\0' : 0;

  inotify = inotify_init ();
  CHECK (inotify >= 0, "inotify init failed");

  ssize_t nread;
  char buff[BUFF_SIZE];
  struct inotify_event *event;

  /* watch dirs recursively */
  add_watch (path);

  for (;;)
    {
      nread = read (inotify, buff, BUFF_SIZE);
      CHECK (nread > 0, "read from inotify instance failed");

      for (char *ptr = buff; ptr < buff + nread;)
        {
          event = (struct inotify_event *)ptr;
          /* inotify_rm_watch (inotify, event->wd); */
          inotify_add_watch (inotify, entries[event->wd], IN_ONLYDIR);
          work (event);
          inotify_add_watch (inotify, entries[event->wd], MASK);
          ptr += sizeof (struct inotify_event) + event->len;
        }
    }

  for (int i = 0; i < WATCH_SIZE; i++)
    free (entries[i]);
}

static void
add_watch (const char *dpath)
{
  DIR *dir = opendir (dpath);
  CHECK (dir != NULL, "open dir failed");

  struct dirent *item;
  char subdir[NAME_SIZE];
  size_t len = strlen (dpath);

  for (item = readdir (dir); item; item = readdir (dir))
    if (item->d_type == DT_DIR)
      {
        if (strcmp (item->d_name, ".") == 0)
          {
            /* watch self */
            int wd = inotify_add_watch (inotify, dpath, MASK);
            CHECK (wd > 0, "watch subdir failed");
            printf ("watching: %s\n", dpath);

            /* save full path */
            char *str = (char *)malloc (len + 1);
            CHECK (strcpy (str, dpath) == str, "copy path failed");
            entries[wd] = str;

            continue;
          }

        if (strcmp (item->d_name, "..") == 0)
          /* skip parent dir */
          continue;

        /* build the full path of subdir */
        size_t item_len = strlen (item->d_name);
        memcpy (subdir, dpath, len);
        subdir[len] = '/';
        memcpy (subdir + len + 1, item->d_name, item_len + 1);

        add_watch (subdir);
      }

  closedir (dir);
}

static void
get_time (char *dest, size_t size)
{
  time_t raw;
  time (&raw);

  struct tm *tm = localtime (&raw);
  size_t ret = strftime (dest, size, "%Y-%m-%d %H:%M:%S", tm);
  CHECK (ret > 0, "strftime failed");
}

static void
work (struct inotify_event *event)
{
  /* ignore files whose name start with `.` */
  if (event->name[0] == '.')
    return;

  printf ("\nfile event: %s/%s\n", entries[event->wd], event->name);

  size_t len = strlen (entries[event->wd]);
  /* do not use event->len directly */
  size_t len2 = strlen (event->name);

  /* check whether it's a markdown file */
  if (strcmp (event->name + len2 - 3, ".md") != 0)
    {
      printf ("    not markdown file: %s\n", event->name);
      return;
    }

  char file_path[NAME_SIZE];
  memcpy (file_path, entries[event->wd], len);
  file_path[len] = '/';
  memcpy (file_path + len + 1, event->name, len2 + 1);

  /* write delay */
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = WRITE_DELAY;
  nanosleep (&ts, NULL);

  FILE *fs = fopen (file_path, "r+");
  CHECK (fs != NULL, "open target file failed");

  size_t line_len;
  char line_buff[LINE_SIZE];
  int front = 0, nline = 1;

  for (;;)
    {
      if (fgets (line_buff, LINE_SIZE, fs) == NULL)
        {
          /* reach the end of the file*/
          printf ("    there is no front-matter\n");
          goto end;
        }

      /* save lbuf size */
      line_len = strnlen (line_buff, LINE_SIZE);

      /* find front-matter */
      if (nline && !strncmp (line_buff, "---", 3))
        front++;

      if (front > 1)
        {
          /* can not find `updated` field */
          printf ("    there is no updated field\n");
          return;
        }

      /* there is a potential problem */
      if (nline && !strncmp (line_buff, "updated", 7))
        break;

      nline = line_buff[line_len - 1] == '\n';
    }

  /* try to update time */
  char time_buff[24];
  get_time (time_buff, 24);
  printf ("    try to update\n");
  fseek (fs, -line_len + 9, SEEK_CUR);

  /* check whether there is enough sapce */
  size_t space = 0;
  for (unsigned i = 9; i < line_len; i++)
    {
      if (line_buff[i] == '\n' || line_buff[i] == 0)
        break;
      space++;
    }

  if (space < 19)
    {
      printf ("    there is no enough sapce to overwrite\n");
      goto end;
    }

  /* try to overwrite time */
  printf ("    new time: %s\n", time_buff);
  CHECK (fputs (time_buff, fs) != EOF, "write target file failed");
  printf ("    update successfully!\n");

end:
  fclose (fs);
}
