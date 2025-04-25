    .section .text
    .global _start

_start:
    li sp, 0x10000
    call test_jalr
    li a7, 1
    li a0, 'D'
    ecall
    li a0, 'O'
    ecall
    li a0, 'N'
    ecall
    li a0, 'E'
    ecall
    li a0, '\n'
    ecall
    li a7, 93
    li a0, 0
    ecall


