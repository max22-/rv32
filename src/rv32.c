#include "rv32.h"
#include "ecall.h"

#ifdef TRACE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define trace(...) printf(__VA_ARGS__)
#else
#define trace(...)
#endif

#define SEXT(x, n) ((x) & (1 << (n - 1)) ? (x) | (0xFFFFFFFF << n) : (x))

#define RD ((instr >> 7) & 0x1f)
#define RS1 ((instr >> 15) & 0x1f)
#define RS2 ((instr >> 20) & 0x1f)
#define IMM_I ((instr >> 20) & 0xfff)
#define SEXT_IMM_I ((int32_t)SEXT(IMM_I, 12))
#define IMM_S (((instr & 0xf80) >> 7) | ((instr & 0xfe000000) >> 20))
#define SEXT_IMM_S ((int32_t)SEXT(IMM_S, 12))
#define IMM_B                                                                  \
  (((instr & 0xf00) >> 7) | ((instr & 0x7e000000) >> 20) |                     \
   ((instr & 0x80) << 4) | ((instr & 0x80000000) >> 19))
#define SEXT_IMM_B ((int32_t)SEXT(IMM_B, 13))
#define IMM_U (instr >> 12)
#define SEXT_IMM_U ((int32_t)SEXT(IMM_U, 20))
#define IMM_J                                                                  \
  (((instr & 0x7fe00000) >> 20) | ((instr & 0x100000) >> 9) |                  \
   (instr & 0xff000) | ((instr & 0x80000000) >> 11))
#define SEXT_IMM_J ((int32_t)SEXT(IMM_J, 20))


/* bus (draft) */
#define LOAD8(addr) (*(uint8_t *)(rv32->mem + addr))
#define LOAD16(addr) (LOAD8(addr) | LOAD8(addr+1) << 8)
#define LOAD32(addr) (LOAD8(addr) | LOAD8(addr+1) << 8 | LOAD8(addr+2) << 16 | LOAD8(addr+3) << 24)
#define STORE8(addr, val) *(uint8_t*)(rv32->mem + addr) = val
#define STORE16(addr, val)            \
  do {                                \
    STORE8(addr, val & 0xff);         \
    STORE8(addr+1, (val>>8) & 0xff);  \
  } while(0)
#define STORE32(addr, val)            \
  do {                                \
    STORE8(addr, val & 0xff);         \
    STORE8(addr+1, (val>>8) & 0xff);  \
    STORE8(addr+2, (val>>16) & 0xff); \
    STORE8(addr+3, (val>>24) & 0xff); \
  } while(0)
/* ************ */

const char *rname[] = {"zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                       "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                       "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                       "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

RV32 *rv32_new(uint32_t mem_size, void *(*calloc_func)(size_t, size_t)) {
  RV32 *rv32 = (RV32 *)calloc_func(1, sizeof(RV32) + mem_size);
  rv32->mem_size = mem_size;
  return rv32;
}

rv32_result_t rv32_cycle(RV32 *rv32) {
  uint32_t instr, addr;
  uint8_t opcode, funct3, funct7;

  if (rv32->pc >= rv32->mem_size)
    return RV32_INVALID_MEMORY_ACCESS;
  instr = LOAD32(rv32->pc);
  opcode = instr & 0x7f;
  funct3 = (instr >> 12) & 0x7;
  funct7 = (instr >> 25) & 0x7f;

  trace("pc=%08x\t", rv32->pc);
  trace("opcode=%02x\t", opcode);

  rv32->r[0] = 0;
  switch (opcode) {

  case 0x33:
    if (funct7 == 0x01) { /* Multiply extension */
      switch (funct3) {
      case 0x0: /* mul */
        rv32->r[RD] =
            (((int64_t)rv32->r[RS1] * (int64_t)rv32->r[RS2]) & 0xFFFFFFFF);
        trace("mul %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      case 0x1: /* mulh */
        rv32->r[RD] =
            ((int64_t)(int32_t)rv32->r[RS1] * (int64_t)(int32_t)rv32->r[RS2]) >>
            32;
        trace("mulh %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      case 0x2: /* mulhsu */
        rv32->r[RD] =
            ((int64_t)(int32_t)rv32->r[RS1] * (int64_t)rv32->r[RS2]) >> 32;
        trace("mulhsu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      case 0x3: /* mulhu */
        rv32->r[RD] = ((uint64_t)rv32->r[RS1] * (uint64_t)rv32->r[RS2]) >> 32;
        trace("mulhu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      case 0x4: /* div */ {
        int32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = 0xFFFFFFFF;
        else if (dividend == 0x80000000 && divisor == -1)
          rv32->r[RD] = dividend; /* overflow */
        else
          rv32->r[RD] = dividend / divisor;
        trace("div %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      }
      case 0x5: /* divu */ {
        uint32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = 0xFFFFFFFF;
        else
          rv32->r[RD] = dividend / divisor;
        trace("divu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      }
      case 0x6: /* rem */ {
        int32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = dividend;
        else if (dividend == 0x80000000 && divisor == -1)
          rv32->r[RD] = 0; /* overflow) */
        else
          rv32->r[RD] = dividend % divisor;
        trace("rem %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      }
      case 0x7: /* remu */ {
        uint32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = dividend;
        else
          rv32->r[RD] = dividend % divisor;
        trace("remu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        break;
      }
      default:
        return RV32_INVALID_INSTRUCTION;
        break;
      }
    } else {
      switch (funct3) {
      case 0x0:
        if (funct7 == 0x00) /* add */
          rv32->r[RD] = rv32->r[RS1] + rv32->r[RS2];
        else if (funct7 == 0x20) /* sub */
          rv32->r[RD] = rv32->r[RS1] - rv32->r[RS2];
        else
          return RV32_INVALID_INSTRUCTION;
        break;
      case 0x4: /* xor */
        rv32->r[RD] = rv32->r[RS1] ^ rv32->r[RS2];
        break;
      case 0x6: /* or */
        rv32->r[RD] = rv32->r[RS1] | rv32->r[RS2];
        break;
      case 0x7: /* and */
        rv32->r[RD] = rv32->r[RS1] & rv32->r[RS2];
        break;
      case 0x1: /* sll */
        rv32->r[RD] = rv32->r[RS1] << rv32->r[RS2];
        break;
      case 0x5:
        if (funct7 == 0x0) /* srl */
          rv32->r[RD] = rv32->r[RS1] >> rv32->r[RS2];
        else if (funct7 == 0x20) /* sra */
          rv32->r[RD] = (int32_t)rv32->r[RS1] >> rv32->r[RS2];
        else
          return RV32_INVALID_INSTRUCTION;
        break;
      case 0x2: /* slt */
        rv32->r[RD] = (int32_t)rv32->r[RS1] < (int32_t)rv32->r[RS2] ? 1 : 0;
        break;
      case 0x3: /* sltu */
        rv32->r[RD] = rv32->r[RS1] < rv32->r[RS2] ? 1 : 0;
        break;
      default:
        return RV32_INVALID_INSTRUCTION;
      }
    }
    rv32->pc += 4;
    break;

  case 0x13:
    switch (funct3) {
    case 0x00: /* addi */
      rv32->r[RD] = rv32->r[RS1] + SEXT_IMM_I;
      trace("addi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x4: /* xori */
      rv32->r[RD] = rv32->r[RS1] ^ SEXT_IMM_I;
      trace("xori %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x6: /* ori */
      rv32->r[RD] = rv32->r[RS1] | SEXT_IMM_I;
      trace("ori %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x7: /* andi */
      rv32->r[RD] = rv32->r[RS1] & SEXT_IMM_I;
      trace("andi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x1: /* slli */
      rv32->r[RD] = rv32->r[RS1] << (IMM_I & 0x1f);
      trace("slli %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
      break;
    case 0x5:
      if (funct7 == 0x00) { /* srli */
        rv32->r[RD] = rv32->r[RS1] >> (IMM_I & 0x1f);
        trace("srli %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
      } else if (funct7 == 0x20) { /* srai */
        rv32->r[RD] = (int32_t)rv32->r[RS1] >> (IMM_I & 0x1f);
        trace("srai %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
      } else
        return RV32_INVALID_INSTRUCTION;
      break;
    case 0x2: /* slti */
      rv32->r[RD] = (int32_t)rv32->r[RS1] < SEXT_IMM_I ? 1 : 0;
      trace("andi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x3: /* sltiu */
      rv32->r[RD] = rv32->r[RS1] < IMM_I ? 1 : 0;
      trace("sltiu %s, %s, %u\n", rname[RD], rname[RS1], IMM_I);
      break;
    default:
      return RV32_INVALID_INSTRUCTION;
    }
    rv32->pc += 4;
    break;

  case 0x3:
    addr = rv32->r[RS1] + SEXT_IMM_I;
    switch (funct3) {
    case 0x0: /* lb */
      if (addr >= rv32->mem_size)
        return RV32_INVALID_MEMORY_ACCESS;
      rv32->r[RD] = SEXT(LOAD8(addr), 8);
      trace("lb %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x1: /* lh */
      if (addr >= rv32->mem_size - 1)
        return RV32_INVALID_MEMORY_ACCESS;
      rv32->r[RD] = SEXT(LOAD16(addr), 16);
      trace("lh %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x2: /* lw */
      if (addr >= rv32->mem_size - 3)
        return RV32_INVALID_MEMORY_ACCESS;
      rv32->r[RD] = LOAD32(addr);
      trace("lw %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x4: /* lbu */
      if (addr >= rv32->mem_size)
        return RV32_INVALID_MEMORY_ACCESS;
      rv32->r[RD] = LOAD8(addr);
      trace("lbu %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x5: /* lhu */
      if (addr >= rv32->mem_size - 1)
        return RV32_INVALID_MEMORY_ACCESS;
      rv32->r[RD] = LOAD16(addr);
      trace("lhu %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    default:
      return RV32_INVALID_INSTRUCTION;
      break;
    }
    rv32->pc += 4;
    break;

  case 0x23:
    addr = rv32->r[RS1] + SEXT_IMM_S;
    switch (funct3) {
    case 0x0: /* sb */
      if (addr >= rv32->mem_size)
        return RV32_INVALID_MEMORY_ACCESS;
      STORE8(addr, rv32->r[RS2] & 0xff);
      trace("sb %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_S);
      break;
    case 0x1: /* sh */
      if (addr >= rv32->mem_size)
        return RV32_INVALID_MEMORY_ACCESS;
      STORE16(addr, rv32->r[RS2] & 0xFFFF);
      trace("sh %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_S);
      break;
    case 0x2: /* sw */
      if (addr >= rv32->mem_size - 3)
        return RV32_INVALID_MEMORY_ACCESS;
      STORE32(addr, rv32->r[RS2]);
      trace("sw %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_S);
      break;
    default:
      return RV32_INVALID_INSTRUCTION;
    }
    rv32->pc += 4;
    break;

  case 0x63:
    switch (funct3) {
    case 0x0: /* beq */
      if (rv32->r[RS1] == rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("beq %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    case 0x1: /* bne */
      if (rv32->r[RS1] != rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("bne %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    case 0x4: /* blt */
      if ((int32_t)rv32->r[RS1] < (int32_t)rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("blt %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    case 0x5: /* bge */
      if ((int32_t)rv32->r[RS1] >= (int32_t)rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("bge %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    case 0x6: /* bltu */
      if (rv32->r[RS1] < rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("bltu %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    case 0x7: /* bgeu */
      if (rv32->r[RS1] >= rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      trace("bgeu %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      break;
    default:
      return RV32_INVALID_INSTRUCTION;
    }
    break;

  case 0x6f: /* jal */
    rv32->r[RD] = rv32->pc + 4;
    rv32->pc += SEXT_IMM_J;
    trace("jal %s, %d\n", rname[RD], SEXT_IMM_J);
    break;

  case 0x67: /* jalr */
    rv32->r[RD] = rv32->pc + 4;
    rv32->pc = rv32->r[RS1] + SEXT_IMM_I;
    trace("jalr %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
    break;

  case 0x37: /* lui */
    rv32->r[RD] = SEXT_IMM_U << 12;
    rv32->pc += 4;
    trace("lui %s, %d\n", rname[RD], SEXT_IMM_U);
    break;

  case 0x17: /* auipc */
    rv32->r[RD] = rv32->pc + (SEXT_IMM_U << 12);
    rv32->pc += 4;
    trace("auipc %s, %d\n", rname[RD], SEXT_IMM_U);
    break;

  case 0x73: /* ecall */
    switch (IMM_I) {
    case 0x0: /* ecall */
      ecall(rv32);
      trace("ecall %d\n", rv32->r[17]);
      break;
    case 0x1: /* ebreak */
#warning ebreak not implemented
      trace("ebreak\n");
      break;
    default:
      return RV32_INVALID_INSTRUCTION;
    }
    rv32->pc += 4;
    break;
  default:
    return RV32_INVALID_OPCODE;
  }
  return RV32_OK;
}
