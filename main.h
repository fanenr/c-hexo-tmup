#ifndef MAIN_H
#define MAIN_H

#include <time.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>

#define NAME_SIZE 256
#define LINE_SIZE 1024
#define BUFF_SIZE 4096
#define WATCH_SIZE 4096
#define MASK IN_CLOSE_WRITE

static void add_watch(const char *dpath);
static void get_time(char *dest, size_t size);
static void work(struct inotify_event *event);

#endif