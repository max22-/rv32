all: rv32 program.bin c_program.bin

rv32: rv32.c rv32.h ecall.c ecall.h
	gcc rv32.c ecall.c -o rv32 -Wall -std=c89 -DUSE_C_STDLIB

program.bin: program.elf
	riscv64-unknown-elf-objcopy program.elf -O binary program.bin

program.elf: program.o
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 program.o -o program.elf -nostdlib -static

program.o: program.s
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 program.s -o program.o

c_program.bin: c_program.elf
	riscv64-unknown-elf-objcopy c_program.elf -O binary c_program.bin

c_program.elf: c_program.c crt0.o
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -nostartfiles -nostdlib -Wl,-T,linker_script.ld c_program.c -o c_program.elf

crt0.o: crt0.s
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 crt0.s -o crt0.o

.PHONY: run disassemble clean

run: rv32 program.bin
	./rv32 program.bin

disassemble: program.elf
	riscv64-unknown-elf-objdump -d program.elf

clean:
	rm -f rv32 program.bin program.elf program.o c_program.elf c_program.bin
