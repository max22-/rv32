CFLAGS = -std=c99 -Wall -pedantic
all: loadelf

loadelf: loadelf.c elf.h
	$(CC) $(CFLAGS) $< -o $@ 

.PHONY: run clean

run: loadelf
	./loadelf ../bin/foo

clean:
	rm loadelf