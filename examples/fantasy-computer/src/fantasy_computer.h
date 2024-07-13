#ifndef FANTASY_COMPUTER_H
#define FANTASY_COMPUTER_H

#include <stdio.h>
#include <stdlib.h>

#include "rv32.h"
#include "shared.h"

#ifdef FANTASY_COMPUTER_IMPLEMENTATION

/* syscalls, mmio *********************************************************** */

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)) { 
    ((uint8_t*)pixels)[addr - 0x80000000] = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t) - (sizeof(uint16_t) - 1)) { 
    *(uint16_t*)((uint8_t*)pixels + addr - 0x80000000) = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t) - (sizeof(uint32_t) - 1)) { 
    *(uint32_t*)((uint8_t*)pixels + addr - 0x80000000) = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}

void ecall(RV32 *rv32) {
  switch (rv32->r[REG_A7]) {
  case 1:
    printf("%c", rv32->r[REG_A0]);
    break;
  case 2:
    printf("%d\n", rv32->r[REG_A0]);
    break;
  case 3: /* get screen width */
    rv32->r[REG_A0] = SCREEN_WIDTH;
    break;
  case 4:
    rv32->r[REG_A0] = SCREEN_HEIGHT;
    break;
  case 5:
    render = true;
    break;
  case 93: /* exit */
    rv32->status = RV32_HALTED;
    break;
  default:
    break;
  }
}

#endif /* FANTASY_COMPUTER_IMPLEMENTATION */
#endif /* FANTASY_COMPUTER_H */