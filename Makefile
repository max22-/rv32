all: rv32

rv32: rv32.c
	gcc rv32.c -o rv32 -Wall -std=c89 -DUSE_C_STDLIB

run: rv32
	./rv32 program.bin

clean:
	rm rv32 -f
