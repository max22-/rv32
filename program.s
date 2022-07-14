	.global _start

_start:
	addi a7, zero, 1
	addi a0, zero, 45
	ecall
	j _start
	addi a7, zero, 93
	addi a0, zero, 0
	ecall
