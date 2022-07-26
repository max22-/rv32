CFLAGS = -Wall -std=c89 -DUSE_C_STDLIB

all: bin/rv32 bin/asm_program.bin bin/c_program.bin

bin/rv32: src/rv32.c src/rv32.h src/ecall.c src/ecall.h
	mkdir -p bin
	gcc src/rv32.c src/ecall.c -o bin/rv32 $(CFLAGS)

bin/asm_program.bin: bin/asm_program.elf
	mkdir -p bin
	riscv64-unknown-elf-objcopy bin/asm_program.elf -O binary bin/asm_program.bin

bin/asm_program.elf: build/asm_program.o
	mkdir -p bin
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 build/asm_program.o -o bin/asm_program.elf -nostdlib -static

build/asm_program.o: examples/asm/asm_program.s
	mkdir -p build
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 examples/asm/asm_program.s -o build/asm_program.o

bin/c_program.bin: bin/c_program.elf
	mkdir -p bin
	riscv64-unknown-elf-objcopy bin/c_program.elf -O binary bin/c_program.bin

bin/c_program.elf: examples/c/c_program.c build/crt0.o
	mkdir -p bin
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -nostartfiles -nostdlib -Wl,-T,examples/c/linker_script.ld examples/c/c_program.c -o bin/c_program.elf

build/crt0.o: examples/c/crt0.s
	mkdir -p build
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 examples/c/crt0.s -o build/crt0.o

.PHONY: run disassemble clean

run_c: bin/rv32 bin/c_program.bin
	bin/rv32 bin/c_program.bin

run_asm: bin/rv32 bin/asm_program.bin
	bin/rv32 bin/asm_program.bin

clean:
	rm -rf bin build
