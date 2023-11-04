CC = gcc
DBG = -ggdb

.PHONY: all
all: main

main: main.o
	$(CC) $(DBG) -o $@ $^

main.o:	main.c
	$(CC) -c $(DBG) -o $@ $^