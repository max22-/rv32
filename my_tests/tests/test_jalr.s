.section .text
.global test_jalr

test_jalr:
    addi sp, sp, -4
    sw ra, 0(sp)
    auipc ra, 0
    jalr ra, 12
    j failure
    j success
    j failure
    j failure
    j failure
    j failure

success:
    li a7, 1
    li a0, 'O'
    ecall
    li a0, 'K'
    ecall
    li a0, '\n'
    ecall
    j done

failure:
    li a7, 1
    li a0, 'K'
    ecall
    li a0, 'O'
    ecall
    li a0, '\n'
    ecall
    j done

done:
    lw ra, 0(sp)
    addi sp, sp, 4
    ret
