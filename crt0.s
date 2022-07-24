	.global _start

_start:
	li sp, 0xFFFF
	jal ra, main
	# a0 contains the return value of main
	li a7, 93
	ecall
