#include "main.h"

int fd;
char *fds[WATCH_SIZE];

int main(int argc, char **argv)
{
    assert(argc == 2 && "must give a pathname");
    char *path = argv[1];
    size_t plen = strnlen(path, NAME_SIZE);
    path[plen - 1] == '/' ? path[plen - 1] = 0 : 0;

    setlocale(LC_ALL, "en_US.UTF-8");
    fd = inotify_init();
    assert(fd >= 0 && "inotify init failed");

    ssize_t nread;
    char buf[BUFF_SIZE];
    struct inotify_event *event;

    /* watch dirs recursively */
    add_watch(path);

    for (;;) {
        nread = read(fd, buf, BUFF_SIZE);
        assert(nread > 0 && "read from inotify instance failed");

        for (char *ptr = buf; ptr < buf + nread;) {
            event = (struct inotify_event *)ptr;
            inotify_add_watch(fd, fds[event->wd], IN_ONLYDIR);
            work(event);
            inotify_add_watch(fd, fds[event->wd], MASK);
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    for (int i = 0; i < WATCH_SIZE; i++)
        free(fds[i]);
}

static void add_watch(const char *dpath)
{
    DIR *dir = opendir(dpath);
    assert(dir != NULL && "open dir failed");

    struct dirent *item;
    char npath[NAME_SIZE];
    size_t len = strnlen(dpath, NAME_SIZE);

    for (item = readdir(dir); item; item = readdir(dir))
        if (item->d_type == DT_DIR) {
            if (!strncmp(item->d_name, ".", NAME_SIZE)) { /* watch self */
                int wd = inotify_add_watch(fd, dpath, MASK);
                assert(wd > 0 && "watch subdir failed");
                printf("watching: %s\n", dpath);

                /* save full path */
                char *str = (char *)malloc(len + 1);
                strncpy(str, dpath, len);
                fds[wd] = str;
                str[len] = 0;

                continue;
            }

            if (!strncmp(item->d_name, "..", NAME_SIZE)) /* skip parent dir */
                continue;

            /* build full child dir path */
            size_t len2 = strnlen(item->d_name, NAME_SIZE);
            assert(len + len2 < NAME_SIZE - 2 && "dirname is too long");

            /* need `/` and `NULL` */
            npath[len] = '/';
            npath[len + len2 + 1] = 0;
            strncpy(npath, dpath, len);
            strncpy(npath + len + 1, item->d_name, len2);

            add_watch(npath);
        }

    closedir(dir);
}

static void get_time(char *dest, size_t size)
{
    time_t raw;
    time(&raw);

    struct tm *tm = localtime(&raw);
    size_t ret = strftime(dest, 48, "%Y-%m-%d %H:%M:%S", tm);
    assert(ret > 0 && "strftime called failed");
}

static void work(struct inotify_event *event)
{
    printf("\nfile event: %s/%s\n", fds[event->wd], event->name);

    size_t len = strnlen(fds[event->wd], NAME_SIZE);
    /* do not use event->len directly */
    size_t len2 = strnlen(event->name, event->len);
    assert(len + len2 < NAME_SIZE - 2 && "filename is too long");

    /* check if it is an md file */
    if (strncmp(event->name + len2 - 3, ".md", 3)) {
        printf("    not markdown file: %s\n", event->name);
        return;
    }

    /* need `/` and `NULL` */
    char fpath[NAME_SIZE];
    fpath[len] = '/';
    fpath[len + len2 + 1] = 0;
    strncpy(fpath, fds[event->wd], len);
    strncpy(fpath + len + 1, event->name, len2);

    FILE *fs = fopen(fpath, "r+");
    assert(fs != NULL && "open target file failed");

    size_t llen;
    char lbuf[LINE_SIZE];
    int flag = 0, nline = 1;

    for (;;) {
        if (fgets(lbuf, LINE_SIZE, fs) == NULL) {
            /* can not find `updated` field */
            printf("    update failed");
            goto end;
        }

        llen = strnlen(lbuf, LINE_SIZE);       /* save lbuf size */

        if (nline && !strncmp(lbuf, "---", 3)) /* find front-matter */
            flag++;
        if (flag > 1) /* reach the end of the front-matter */
            goto end;
        if (nline && !strncmp(lbuf, "updated", 7)) /* a potential problem */
            break;

        nline = lbuf[llen - 1] == '\n';
    }

    char tbuf[32];

    printf("    try to update\n");
    fseek(fs, -llen + 9, SEEK_CUR);
    get_time(tbuf, 32);
    printf("    new time: %s\n", tbuf);

    assert(fputs(tbuf, fs) != EOF && "write target file failed");
    printf("    update successfully!\n");

end:
    fclose(fs);
}