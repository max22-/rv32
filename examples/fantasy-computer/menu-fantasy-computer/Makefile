EXE = menu.elf
CC = riscv32-unknown-elf-gcc
INCLUDE_DIRS = -Isrc
CFLAGS = -march=rv32im -mabi=ilp32 -pedantic -Wall -MMD -MP $(INCLUDE_DIRS) -g
LDFLAGS = -march=rv32im -mabi=ilp32 -nostartfiles -Wl,-T,linker_script.ld
SRCS = $(shell find src -name *.c)
OBJS = $(SRCS:%=build/%.o)
DEPS = $(OBJS:.o=.d)

all: bin/$(EXE)

bin/$(EXE): $(OBJS)
	mkdir -p bin
	$(CC) $^ $(LDFLAGS) -o $@ 

build/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

crt0.o: crt0.s
	riscv32-unknown-elf-as -march=rv32im -mabi=ilp32 $< -o $@

.PHONY: run clean

run: bin/$(EXE)
	gdb-multiarch -batch -ex "set serial baud 115200" -ex "target remote /dev/ttyUSB1" -ex load -ex continue -ex interrupt -ex quit bin/menu.elf

clean:
	rm -rf bin build

-include $(DEPS)
