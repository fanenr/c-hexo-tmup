MODE = debug

include config.mk

.PHONY: all
all: tmup

tmup: main.o
	gcc -o $@ $^ $(LDFLAGS)

main.o:	main.c config.h
	gcc -c $< $(CFLAGS)

.PHONY: clean
clean:
	-rm -f *.o tmup
