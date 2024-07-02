EXE = fantasy-computer
INCLUDE_DIRS = -I../../src
CFLAGS = -std=c99 -pedantic -Wall -MMD -MP -DLITTLE_ENDIAN_HOST $(shell pkg-config --cflags sdl2) $(INCLUDE_DIRS) -g
LDFLAGS = $(shell pkg-config --libs sdl2) -lpthread
SRCS = $(shell find src -name *.c)
OBJS = $(SRCS:%=build/%.o)
DEPS = $(OBJS:.o=.d)

all: bin/$(EXE)

bin/$(EXE): $(OBJS)
	mkdir -p bin
	$(CC) $^ -o $@ $(LDFLAGS)

build/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run clean

run: bin/$(EXE)
	./bin/$(EXE) c_program.bin

clean:
	rm -rf bin build

-include $(DEPS)
