#ifdef USE_C_STDLIB
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* #define trace(...) printf(__VA_ARGS__) */
#define trace(...)

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
#define RS2(instr) ((instr >> 20) & 0x1f)
#define IMM_I(instr) ((instr >> 20) & 0xfff)
#define IMM_B(instr)			\
  (((instr & 0xf00) >> 7)		\
   | ((instr & 0x7e000000) >> 20)	\
   | ((instr & 0x80) << 4)		\
   | ((instr & 0x80000000) >> 19))
#define IMM_J(instr)			\
  (((instr & 0x7fe00000) >> 20)		\
   | ((instr & 0x100000) >> 9)		\
   | (instr & 0xff000)			\
   | ((instr & 0x80000000) >> 11))	
							

#define SEXT(x, n) (x & (1 << n) ? x | (0xFFFFFFFF << n) : x)

typedef struct {
  uint32_t r[32], pc;
  uint8_t mem[1];
} RV32;

const char *rname[] = {
  "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

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

void rv32_dump(RV32 *rv32)
{
  int i;
  for(i = 0; i < 32; i++) {
    printf("%s=0x%08x\t", rname[i], rv32->r[i]);
    if((i+1) % 8 == 0)
      printf("\n");
  }
  printf("pc = 0x%08x\n", rv32->pc);
}

void rv32_cycle(RV32 *rv32)
{
  uint32_t opcode, rd, rs1, rs2, imm;
  uint32_t instr = *(uint32_t*)&rv32->mem[rv32->pc];
  rv32->r[0] = 0;
  opcode = OPCODE(instr);
  switch(opcode) {
  case 0x13:
    switch(FUNCT3(instr)) {
    case 0x00:
      rd = RD(instr);
      rs1 = RS1(instr);
      imm = SEXT(IMM_I(instr), 12);
      rv32->r[rd] = rv32->r[rs1] + (int32_t)imm;
      trace("addi %s, %s, 0x%x\n", rname[rd], rname[rs1], imm);
      break;
    }
    rv32->pc += 4;
    break;
  case 0x63:
    switch(FUNCT3(instr)) {
    case 0x0: /* beq */
      rs1 = RS1(instr);
      rs2 = RS2(instr);
      imm = SEXT(IMM_B(instr), 13);
      if(rv32->r[rs1] == rv32->r[rs2])
	rv32->pc += (int32_t)imm;
      else
	rv32->pc += 4;
      trace("beq %s, %s, 0x%x\n", rname[rs1], rname[rs2], imm);
      break;
    default:
      error("Invalid instruction");
      break;
    }
    break;
  case 0x6f: /* jal */
    rd = RD(instr);
    imm = SEXT(IMM_J(instr), 20);
    rv32->r[rd] = rv32->pc + 4;
    rv32->pc += (int32_t)imm;
    trace("jal %s, 0x%x\n", rname[rd], imm);
    break;
  case 0x73: /* ecall */
    if(rv32->r[17] == 1)
      printf("%d\n", rv32->r[10]);
    else if(rv32->r[17] == 93)
      exit(rv32->r[10]);
    rv32->pc += 4;
    break;
  default:
    error("Invalid opcode");
    break;
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

  while(1)
    rv32_cycle(rv32);
  
  free(rv32);
  return 0;
}
