CC   = gcc
MODE = debug

include config.mk

.PHONY: all
all: tmup

tmup: main.o mstr.o
	$(CC) -o $@ $^ $(LDFLAGS)

mstr.o: mstr.c mstr.h
main.o:	main.c util.h mstr.h config.h

.PHONY: clean
clean:
	-rm -f *.o tmup
