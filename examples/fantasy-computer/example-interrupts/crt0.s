	.global _start

_start:
	la sp, __stack_top
	jal ra, main
	# a0 contains the return value of main
	li a7, 93
	ecall
