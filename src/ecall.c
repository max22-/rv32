#ifdef USE_C_STDLIB
#include <stdio.h>
#endif

#include "rv32.h"

void ecall(RV32 *rv32)
{
  switch(rv32->r[17]) { /* switch(a7) */
  case 1:
    printf("%c", rv32->r[10]);
  case 2:
    printf("%d\n", rv32->r[10]);
    break;
  case 93:
    rv32->halted = 1;
    break;
  default:
    break;
  }
}
