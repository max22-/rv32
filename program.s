	.global _start

_start:
	li t0, 0
	li t1, 10
	li t2, 10
	li t3, 1
loop:
	addi t0, t0, 1
	beq t0, t1, end

	addi a7, zero, 1
	addi a0, t0, 0
	add a0, a0, t2
	sub a0, a0, t3
	ecall
	j loop

end:	
	addi a7, zero, 93
	addi a0, zero, 0
	ecall
