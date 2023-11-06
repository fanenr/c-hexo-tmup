CC  := gcc
DBG := -ggdb
APP := tmup

.PHONY: all
all: $(APP)

$(APP): main.o
	$(CC) -o $(APP) $^

main.o:	main.c main.h
	$(CC) -c $(DBG) $<

.PHONY: clean
clean:
	-rm -f $(APP) main.o