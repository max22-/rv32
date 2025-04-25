TEST_OBJS = $(addsuffix .o,$(basename $(wildcard tests/*.s)))

all: tests.bin

tests.bin: tests.elf
	riscv64-unknown-elf-objcopy tests.elf -O binary tests.bin

tests.elf: start.o $(TEST_OBJS)
	riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -ffreestanding -nostdlib -Wl,-Bstatic,-T,linker.ld -o $@ start.o $(TEST_OBJS)

tests/%.o: tests/%.s
	riscv64-unknown-elf-gcc -c -march=rv32im -mabi=ilp32 -o $@  $<

start.o: start.s
	riscv64-unknown-elf-gcc -c -march=rv32im -mabi=ilp32 -o $@ $<

clean:
	rm -f tests.bin tests.elf start.o tests/*.o
