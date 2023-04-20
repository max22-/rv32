#include "rv32.h"
#include <stdio.h>

void ecall(RV32 *rv32) {
  switch (rv32->r[REG_A7]) {
  case 1:
    printf("%c", rv32->r[REG_A0]);
    break;
  case 2:
    printf("%d\n", rv32->r[REG_A0]);
    break;
  case 93: /* exit */
    rv32->status = RV32_HALTED;
    break;
  default:
    break;
  }
}
