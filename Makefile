LDFLAGS   = -g
CFLAGS    = -Wall -Wextra -Werror -ggdb3 -std=gnu11

.PHONY: all
all: tmup

tmup: main.o
	gcc -o $@ $^ $(LDFLAGS)

main.o:	main.c config.h
	gcc -c $< $(CFLAGS)

.PHONY: clean
clean:
	-rm -f *.o tmup
