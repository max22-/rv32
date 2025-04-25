    .section .text
    .global _start

_start:
    li sp, 0x10000
    call test_jalr
    li a7, 93
    li a0, 0
    ecall


