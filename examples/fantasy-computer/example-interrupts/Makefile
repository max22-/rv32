all: example.bin

example.bin: example.elf
	riscv64-unknown-elf-objcopy $< -O binary $@

example.elf: example.c crt0.o
	riscv64-unknown-elf-gcc -g -march=rv32im_zicsr -mabi=ilp32 -nostartfiles -Wl,-T,linker_script.ld $< -o $@

crt0.o: crt0.s
	riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 $< -o $@

clean:
	rm -f *.bin *.elf *.o
