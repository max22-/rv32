CC = riscv32-unknown-elf-gcc
CFLAGS = -Os

all: hello.bin emulator

hello.bin: hello.elf
	riscv32-unknown-elf-objcopy hello.elf -O binary hello.bin

hello.elf: hello.c stubs.c ecall.c ecall.h rv32.c rv32.h
	$(CC) $(CFLAGS) --specs=nano.specs -Wl,--section-start=.text=0 hello.c stubs.c -o hello.elf

emulator: emulator.c rv32.c rv32.h ecall.h ecall.c
	gcc -std=c99 -pedantic -Wall -O2 -Wall emulator.c rv32.c ecall.c -o emulator

.PHONY: run disassemble clean

run: emulator hello.bin
	./emulator hello.bin

disassemble: hello.elf
	riscv32-unknown-elf-objdump -d hello.elf | less

clean:
	rm -f hello.elf hello.bin emulator
