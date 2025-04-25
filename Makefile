CFLAGS = -Wall -std=c89 -pedantic -g

all: bin/rv32 bin/asm_program.bin bin/c_program.bin

bin/rv32: examples/embedding/emulator.c examples/embedding/ecall.c src/rv32.h
	mkdir -p bin
	gcc -Isrc examples/embedding/emulator.c examples/embedding/ecall.c -o bin/rv32 $(CFLAGS)

bin/asm_program.bin: bin/asm_program.elf
	mkdir -p bin
	riscv64-unknown-elf-objcopy $< -O binary $@

bin/asm_program.elf: build/asm_program.o
	mkdir -p bin
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 $< -o $@ -nostdlib -static

build/asm_program.o: examples/asm/asm_program.s
	mkdir -p build
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 $< -o $@

bin/hello.bin: bin/hello.elf
	mkdir -p bin
	riscv64-unknown-elf-objcopy $< -O binary $@

bin/hello.elf: build/hello.o
	mkdir -p bin
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 $< -o $@ -nostdlib -static

build/hello.o: examples/asm/hello_world/hello.s
	mkdir -p build
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 $< -o $@

bin/c_program.bin: bin/c_program.elf
	mkdir -p bin
	riscv64-unknown-elf-objcopy $< -O binary $@

bin/c_program.elf: examples/c/c_program.c build/crt0.o
	mkdir -p bin
	riscv64-unknown-elf-gcc -g -march=rv32im -mabi=ilp32 -nostartfiles -nostdlib -Wl,-T,examples/c/linker_script.ld $< -o $@

build/crt0.o: examples/c/crt0.s
	mkdir -p build
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 $< -o $@

tests/tests.bin:
	make -C tests

.PHONY: run test disassemble clean

test: bin/rv32 tests/tests.bin
	./bin/rv32 tests/tests.bin

run_c: bin/rv32 bin/c_program.bin
	bin/rv32 bin/c_program.bin

run_asm: bin/rv32 bin/asm_program.bin
	bin/rv32 bin/asm_program.bin

run_hello: bin/rv32 bin/hello.bin
	bin/rv32 bin/hello.bin

clean:
	rm -rf bin build
	make -C tests clean