CC  := gcc
DBG := -ggdb
APP := tmup

.PHONY: all
all: main

main: main.o
	$(CC) $(DBG) -o $(APP) $^

main.o:	main.c main.h
	$(CC) -c $(DBG) -o $@ $<

.PHONY: clean
clean:
	-rm -f $(APP) main.o