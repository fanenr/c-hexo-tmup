CC = gcc
DBG = -ggdb

.PHONY: all
all: main

main: main.o
	$(CC) $(DBG) -o $@ $^

main.o:	main.c main.h
	$(CC) -c $(DBG) -o $@ $<

.PHONY: clean
clean:
	-rm -f main main.o