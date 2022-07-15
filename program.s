	.global _start

_start:
	li t0, 0
	li t1, 10
loop:
	addi t0, t0, 1
	beq t0, t1, end

	addi a7, zero, 1
	addi a0, t0, 0
	ecall
	j loop

end:	
	addi a7, zero, 93
	addi a0, zero, 0
	ecall
