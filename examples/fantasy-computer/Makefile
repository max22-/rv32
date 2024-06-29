CFLAGS = -std=c99 -pedantic -Wall -I../src -DLITTLE_ENDIAN_HOST $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

all: fantasy-computer

fantasy-computer: main.c ../../src/rv32.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: run clean

run: fantasy-computer
	./fantasy-computer c_program.bin

clean:
	rm -f fantasy-computer
