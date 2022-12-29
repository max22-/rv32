#include <stdio.h>
#include "rv32.h"
#include "ecall.h"

void ecall(RV32 *rv32)
{
  switch(rv32->r[REG_A7]) {
  case 1:
  case 64:
    printf("write(%d, 0x%08x, %d)\n",
	   rv32->r[REG_A0], rv32->r[REG_A1], rv32->r[REG_A2]);
    fwrite(&rv32->mem[rv32->r[REG_A1]], rv32->r[REG_A2], 1, stdout);
    break;
  default:
    fprintf(stderr, "Unknown system call: %d\n", rv32->r[REG_A7]);
  }
}
