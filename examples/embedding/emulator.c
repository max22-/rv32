#include <stdio.h>
#include <stdlib.h>
#define RV32_IMPLEMENTATION
#include "rv32.h"

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr==0x80000000) { 
    printf("%c", val); 
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_MMIO_ERR; }

int main(int argc, char *argv[]) {
  FILE *f;
  RV32 *rv32;
  size_t fsize;
  const size_t ram_size = 0x20000004; /* The tests write a byte at 0x20000000
                                        so the memory needs to be this big ! */
  uint8_t *memory = NULL;

  if (argc < 2) {
    fprintf(stderr, "Please provide a program to run.\n");
    return 1;
  }
  f = fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "Failed to open program %s\n", argv[1]);
    return 1;
  }
  memory = (uint8_t*)malloc(RV32_NEEDED_MEMORY(ram_size));
  if(!memory) {
    fprintf(stderr, "Failed to allocate memory.\n");
    return 1;
  }
  rv32 = rv32_new(memory, ram_size);
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  if (fsize > ram_size) {
    fprintf(stderr, "Program too big.\n");
    fclose(f);
    free(memory);
    return 1;
  }
  fseek(f, 0, SEEK_SET);
  if (fread(rv32->mem, fsize, 1, f) != 1) {
    fclose(f);
    fprintf(stderr, "Failed to read program\n");
    free(memory);
    return 1;
  }
  fclose(f);

  while (rv32->status == RV32_RUNNING) {
    rv32_cycle(rv32);
    switch(rv32->status) {
      case RV32_RUNNING:
        break;
      case RV32_HALTED:
        break;
      case RV32_EBREAK:
        fprintf(stderr, "ebreak at pc=%08x\n", rv32->pc);
        break;
      default:
        fprintf(stderr, "Error %s at pc=%08x\n", rv32_status_name[rv32->status], rv32->pc);
        fprintf(stderr, "instruction = 0x%08x\n", *(uint32_t *)&rv32->mem[rv32->pc]);
    }
  }
  int rc = rv32->status == RV32_HALTED ? rv32->r[REG_A0] : 1;
  /*
  printf("exit status: %d\n", rv32->r[REG_A0]);
  printf("\n");
  */
  free(memory);
  return rc;
}
