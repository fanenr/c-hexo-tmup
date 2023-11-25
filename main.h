#ifndef MAIN_H
#define MAIN_H

#define NAME_SIZE 256
#define LINE_SIZE 1024
#define BUFF_SIZE 4096
#define WATCH_SIZE 4096

#define MASK IN_CLOSE_WRITE

#define CHECK(expr, msg)                                                       \
    if (!(expr)) {                                                             \
        fprintf(stderr, "error on line: %d\nexpr: %s\nmsg : %s\n", __LINE__,   \
                #expr, msg);                                                   \
        exit(0);                                                               \
    }

#endif
