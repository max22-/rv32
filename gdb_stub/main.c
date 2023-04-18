#include <stdio.h>
#define RSP_IMPLEMENTATION
#define RV32_IMPLEMENTATION
#include "rsp.h"

rv32_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr==0x80000000) { 
    printf("%c", val); 
    return RV32_OK;
  }
  return RV32_INVALID_MEMORY_ACCESS;
}
rv32_result_t mmio_store16(uint32_t addr, uint16_t val) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_INVALID_MEMORY_ACCESS; }

void ecall(RV32 *rv32) {}

int main(int argc, char *argv[])
{
  RV32 *rv32;
  int c;

  rv32 = rv32_new(0x1000, calloc);
  if (!rv32) {
    fprintf(stderr, "Not enough memory\n");
    return 1;
  }

  while((c = fgetc(stdin)) != EOF)
    rsp_handle_byte(rv32, c);

  return 0;
}
