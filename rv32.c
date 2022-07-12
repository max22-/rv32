#ifdef USE_C_STDLIB
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void error(const char *msg)
{
  fprintf(stderr,"%s\n",  msg);
  exit(1);
}

#endif

#define OPCODE(instr) (instr & 0x7f)
#define RD(instr) ((instr >> 7) & 0x1f)
#define FUNCT3(instr) ((instr >> 12) & 0x7)
#define RS1(instr) ((instr >> 15) & 0x1f)
#define I_IMM(instr) ((instr >> 20) & 0xfff)
#define J_IMM(instr) ((instr >> 12) & 0x7ffff)

typedef struct {
  uint32_t r[32], pc;
  uint8_t mem[1];
} RV32;

RV32 *rv32_new(uint32_t mem_size)
{
  RV32 *rv32 = (RV32*)calloc(1, sizeof(RV32) + mem_size);
  return rv32;
}

void rv32_free(RV32 *rv32)
{
  if(rv32 != NULL)
    free(rv32);
}

void rv32_cycle(RV32 *rv32)
{
  uint32_t instr = *(uint32_t*)&rv32->mem[rv32->pc];
  rv32->pc += 4;
  printf("%08x\n", instr);
  printf("\topcode = 0x%x\n", OPCODE(instr));
  if(OPCODE(instr) == 0x13) {
    printf("\trd=x%d\n", RD(instr));
    printf("\tfunct3=%x\n", FUNCT3(instr));
    printf("\trs1=x%d\n", RS1(instr));
    printf("\timm=%d\n", I_IMM(instr));
  } else if(OPCODE(instr) == 0x6f) {
    printf("\trd=x%d\n", RD(instr));
    printf("\timm=0x%x\n", J_IMM(instr));
  }
  
}

int main(int argc, char *argv[])
{
  FILE *f;
  RV32 *rv32;
  size_t fsize;
  const size_t memsize = 65536;
  
  if(argc < 2)
    error("Please provide a program to run.");
  f = fopen(argv[1], "r");
  if(!f)
    error("Failed to open program.");
  rv32 = rv32_new(memsize);
  if(!rv32)
    error("Not enough memory");
  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  if(fsize > memsize)
    error("Program too big.");
  fseek(f, 0, SEEK_SET);
  if(fread(rv32->mem, fsize, 1, f) != 1) {
    fclose(f);
    error("Failed to read program");
  }
  fclose(f);

  rv32_cycle(rv32);
  rv32_cycle(rv32);
  rv32_cycle(rv32);
  
  free(rv32);
  return 0;
}
