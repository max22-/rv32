#include <stdio.h>
#include <stdlib.h>
#include "rv32.h"

int main(int argc, char *argv[])
{
  FILE *f;
  RV32 *rv32;
  size_t fsize;
  const size_t memsize = 65536;
  rv32_result_t res;
  
  if(argc < 2) {
    fprintf(stderr, "Please provide a program to run.\n");
    return 1;
  }
  f = fopen(argv[1], "r");
  if(!f) {
    fprintf(stderr, "Failed to open program %s\n", argv[1]);
    return 1;
  }
  rv32 = rv32_new(memsize, calloc);
  if(!rv32) {
    fprintf(stderr, "Not enough memory\n");
    return 1;
  }
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  if(fsize > memsize) {
    fprintf(stderr, "Program too big.\n");
  }
  fseek(f, 0, SEEK_SET);
  if(fread(rv32->mem, fsize, 1, f) != 1) {
    fclose(f);
    fprintf(stderr, "Failed to read program\n");
  }
  fclose(f);

  rv32->r[REG_SP] = 60000;
  rv32->pc = 0x60;

  while(!rv32->halted) {
    if((res = rv32_cycle(rv32)) != RV32_OK) {
      fprintf(stderr, "Error %d at pc=%08x\n", res, rv32->pc);
      fprintf(stderr, "instr = %08x\n", *(uint32_t*)&rv32->mem[rv32->pc]);
      break;
    }
  }


  /*
  for(i = 0; i < 0x104; i++) {
    if(i%16 == 0)
      printf("%08x\t", i);
    printf("%02x ", rv32->mem[i]);
    if((i+1) % 16 == 0)
      printf("\n");
  }
  */
  
  printf("\n");
  free(rv32);
  return 0;
}
