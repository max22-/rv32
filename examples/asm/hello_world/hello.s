.global _start

_start:
    li a7, 1
    la t0, hello

loop:
    li t1, 0
    lw a0, 0(t0)
    beq a0, t1, end
    ecall
    addi t0, t0, 1
    j loop
    
end:
    li a7, 93
    li a0, 0
    ecall    


hello: .asciz "Hello, world!\n"
