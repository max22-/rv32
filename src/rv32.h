#ifndef INCLUDE_RV32_H
#define INCLUDE_RV32_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  RV32_RUNNING,
  RV32_HALTED,
  RV32_BREAKPOINT,
  RV32_EBREAK,
  RV32_INVALID_INSTRUCTION,
  RV32_INVALID_MEMORY_ACCESS
} rv32_status_t;

typedef struct {
  uint32_t mem_size;
  rv32_status_t status;
  uint8_t bp_mask; /* breakpoint enabled if bit enabled */
  uint32_t bp[8]; /* breakpoints */
  uint32_t r[32], pc;
  /* CSR */
  uint32_t 
    mstatus,  /* machine status (contains the MIE bit (Machine Interrupt Enable)) */
    mie,  /* machine interrupt enable register (which type of interrupts are enabled) */
    mip, /* machine interrupt pending register */
    mtvec, /* machine trap-handler base address */
    mepc, /* machine exception program counter */
    mcause; /* machine trap cause*/
  /* *** */
  uint8_t wfi; /* wait for interrupt */
  struct {
    uint32_t ie; /* interrupt enabled */
    uint32_t ip; /* interrupt pending */
  } interrupt_controller;
  uint8_t mem[1];
} RV32;

enum rv32_register {
  REG_ZERO,
  REG_RA,
  REG_SP,
  REG_GP,
  REG_TP,
  REG_T0,
  REG_T1,
  REG_T2,
  REG_S0,
  REG_FP = 8,
  REG_S1,
  REG_A0,
  REG_A1,
  REG_A2,
  REG_A3,
  REG_A4,
  REG_A5,
  REG_A6,
  REG_A7,
  REG_S2,
  REG_S3,
  REG_S4,
  REG_S5,
  REG_S6,
  REG_S7,
  REG_S8,
  REG_S9,
  REG_S10,
  REG_S11,
  REG_T3,
  REG_T4,
  REG_T5,
  REG_T6
};

/* Gives the amount of memory needed for the RAM + the struct */
#define RV32_NEEDED_MEMORY(bytes) (sizeof(RV32) + bytes)

RV32 *rv32_new(void *memory, uint32_t mem_size);
void rv32_resume(RV32 *rv32);
void rv32_cycle(RV32 *rv32);
int rv32_set_breakpoint(RV32*, uint32_t addr);
int rv32_clear_breakpoint(RV32*, uint32_t addr);
extern void ecall(RV32 *rv32);

typedef enum {
  RV32_MMIO_OK,
  RV32_MMIO_ERR
} rv32_mmio_result_t;

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret);
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret);
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret);

rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val);
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val);
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val);

#ifdef RV32_IMPLEMENTATION

#ifdef TRACE
#include <stdio.h>
#include <stdlib.h>
#define trace(...) fprintf(stderr, __VA_ARGS__)
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

#if defined(LITTLE_ENDIAN_HOST)
#define LOAD8(addr) (*(uint8_t *)(rv32->mem + (addr)))
#define LOAD16(addr) (*(uint16_t *)(rv32->mem + (addr)))
#define LOAD32(addr) (*(uint32_t *)(rv32->mem + (addr)))

#define STORE8(addr, val)                                                      \
  do {                                                                         \
    *(uint8_t *)(rv32->mem + (addr)) = val;                                    \
  } while (0)
#define STORE16(addr, val)                                                     \
  do {                                                                         \
    *(uint16_t *)(rv32->mem + (addr)) = val;                                   \
  } while (0)
#define STORE32(addr, val)                                                     \
  do {                                                                         \
    *(uint32_t *)(rv32->mem + (addr)) = val;                                   \
  } while (0)

#elif defined(BIG_ENDIAN_HOST)
#define LOAD8(addr) (*(uint8_t *)(rv32->mem + (addr)))
#define LOAD16(addr) (LOAD8(addr) | LOAD8((addr) + 1) << 8)
#define LOAD32(addr)                                                           \
  (LOAD8(addr) | LOAD8((addr) + 1) << 8 | LOAD8((addr) + 2) << 16 |            \
   LOAD8((addr) + 3) << 24)
#define STORE8(addr, val)                                                      \
  do {                                                                         \
    *(uint8_t *)(rv32->mem + (addr)) = val;                                    \
  } while (0)
#define STORE16(addr, val)                                                     \
  do {                                                                         \
    STORE8(addr, (val)&0xff);                                                  \
    STORE8(addr + 1, ((val) >> 8) & 0xff);                                     \
  } while (0)
#define STORE32(addr, val)                                                     \
  do {                                                                         \
    STORE8(addr, (val)&0xff);                                                  \
    STORE8(addr + 1, ((val) >> 8) & 0xff);                                     \
    STORE8(addr + 2, ((val) >> 16) & 0xff);                                    \
    STORE8(addr + 3, ((val) >> 24) & 0xff);                                    \
  } while (0)
#else
#error "Please define LITTLE_ENDIAN_HOST or BIG_ENDIAN_HOST macro"
#endif

/* CSR stuff **************************************************************** */

#define CSR_CLEAR_BIT(csr, bit) do { (csr) &= ~(1<< (bit)); } while(0)
#define CSR_SET_BIT(csr, bit, val) do { (csr) = ((csr) & ~(1 << bit)) | ((!!val) << bit); } while(0)
#define CSR_GET_BIT(csr, bit) ((csr) >> (bit) & 0x1)

/* mstatus */
#define CSR_MSTATUS_MIE 3
#define CSR_MSTATUS_MPIE 7

/* mie */
#define CSR_MIE_MSIE 3 /* machine software interrupt enable*/
#define CSR_MIE_MTIE 7 /* machine timer interrupt enable */
#define CSR_MIE_MEIE 11 /* machine external interrupt enable */

/* mip */
#define CSR_MIP_MSIP 3 /* machine software interrupt enable */
#define CSR_MIP_MTIP 7 /* machine timer interrupt enable */
#define CSR_MIP_MEIP 11 /* machine external interrupt enable */

/* mcause */
#define CSR_MCAUSE_INTERRUPT (1 << 31) /* bit set when the exception is 
                                          triggered by an interrupt */
#define CSR_MCAUSE_MSI (0x80000003)
#define CSR_MCAUSE_MTI (0x80000007)
/* #define CSR_MCAUSE_MEI (0x8000000b) */ /* not used in this implementation */

/* ************************************************************************** */

const char *rname[] = {"zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                       "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                       "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                       "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

RV32 *rv32_new(void *memory, uint32_t mem_size) {
  RV32 *rv32 = (RV32 *)memory;
  uint32_t i;
  for(i = 0; i < sizeof(RV32); i++)
      *((uint8_t*)rv32 + i) = 0;
  rv32->mem_size = mem_size;
  return rv32;
}

void rv32_resume(RV32 *rv32) {
  if(rv32->status == RV32_BREAKPOINT)
    rv32->status = RV32_RUNNING;
}

static int rv32_claim_irq(RV32 *rv32, uint32_t *irq) {
  uint32_t i;
  for(i = 0; i < 8 * sizeof(uint32_t); i++) {
    if(rv32->interrupt_controller.ie & rv32->interrupt_controller.ip) {
      *irq = i;
      rv32->interrupt_controller.ip &= ~(1 << i);
      return 1;
    }
  }
  return 0;
}

void rv32_handle_interrupt(RV32 *rv32) {
  if(!CSR_GET_BIT(rv32->mstatus, CSR_MSTATUS_MIE))
    return;
  
  switch(rv32->mie & rv32->mip) {
    case 1<<CSR_MIE_MSIE: /* not implemented yet */
      rv32->mcause = CSR_MCAUSE_MSI;
      break;
    case 1<<CSR_MIE_MTIE: /* not implemented yet */
      rv32->mcause = CSR_MCAUSE_MTI;
      break;
    case 1<<CSR_MIE_MEIE: {
      uint32_t irq;
      if(!rv32_claim_irq(rv32, &irq))
        return;
      rv32->mcause = CSR_MCAUSE_INTERRUPT | (16 + irq);
      break;
    }
    default:
      return;
  }

  rv32->mepc = rv32->pc;
  CSR_SET_BIT(rv32->mstatus, CSR_MSTATUS_MPIE, CSR_GET_BIT(rv32->mstatus, CSR_MSTATUS_MIE));
  CSR_SET_BIT(rv32->mstatus, CSR_MSTATUS_MIE, 0);
  uint32_t mask;
  mask = ~(0x3);
  const uint32_t base_addr = rv32->mtvec & mask;
  const int vectored = rv32->mtvec & 1;
  if(vectored) {
    rv32->pc = base_addr + 4 * (rv32->mcause & ~(1 << 31));
  } else {
    rv32->pc = rv32->mtvec;
  }
  rv32->wfi = 0;
}

static uint32_t rv32_read_csr(RV32 *rv32, uint32_t csr) {
  switch(csr) {
    case 0x300:
      return rv32->mstatus;
    case 0x304:
      return rv32->mie;
    case 0x305:
      return rv32->mtvec;
    case 0x341:
      return rv32->mepc;
    case 0x342:
      return rv32->mcause;
    case 0x344:
      return rv32->mip;
    default:
      return 0;
  }
}

static void rv32_write_csr(RV32 *rv32, uint32_t csr, uint32_t val) {
  switch(csr) {
    case 0x300:
      rv32->mstatus = val;
      break;
    case 0x304:
      rv32->mie = val;
      break;
    case 0x305:
      rv32->mtvec = val;
      break;
    case 0x341:
      rv32->mepc = val;
      break;
    case 0x342:
      rv32->mcause = val;
      break;
    case 0x344:
      rv32->mip = val;
      break;
    default:
      break;
  }
}

void rv32_trigger_external_interrupt(RV32 *rv32, uint32_t irq) {
  if(irq >= sizeof(uint32_t)) /* IRQ between 0 and 31 only */
    return;
  CSR_SET_BIT(rv32->mip, CSR_MIP_MEIP, 1);
  rv32->interrupt_controller.ip |= 1 << irq;
}

void rv32_cycle(RV32 *rv32) {
  uint32_t instr, addr;
  uint8_t opcode, funct3, funct7;
  uint8_t tmp8;
  uint16_t tmp16;
  uint32_t tmp32;
  int i;

  if(CSR_GET_BIT(rv32->mstatus, CSR_MSTATUS_MIE) && (rv32->mie & rv32->mip))
    rv32_handle_interrupt(rv32);
  if(rv32->wfi)
    return;
  if(rv32->status != RV32_RUNNING)
    return;
  if (rv32->pc >= rv32->mem_size) {
    rv32->status = RV32_INVALID_MEMORY_ACCESS;
    return;
  }
  else if(rv32->bp_mask) {
    for(i = 0; i < 8; i++) {
      if(rv32->bp_mask & (1<<i) && rv32->pc == rv32->bp[i]) {
        rv32->status = RV32_BREAKPOINT;
        return;
      }
    }
  }
  instr = LOAD32(rv32->pc);
  opcode = instr & 0x7f;
  funct3 = (instr >> 12) & 0x7;
  funct7 = (instr >> 25) & 0x7f;

  trace("pc=%08x\t", rv32->pc);

  rv32->r[REG_ZERO] = 0;
  switch (opcode) {

  case 0x33:
    if (funct7 == 0x01) { /* Multiply extension */
      switch (funct3) {
      case 0x0: /* mul */
        trace("mul %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] =
            (((int64_t)rv32->r[RS1] * (int64_t)rv32->r[RS2]) & 0xFFFFFFFF);
        break;
      case 0x1: /* mulh */
        trace("mulh %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] =
            ((int64_t)(int32_t)rv32->r[RS1] * (int64_t)(int32_t)rv32->r[RS2]) >>
            32;
        break;
      case 0x2: /* mulhsu */
        trace("mulhsu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] =
            ((int64_t)(int32_t)rv32->r[RS1] * (int64_t)rv32->r[RS2]) >> 32;
        break;
      case 0x3: /* mulhu */
        trace("mulhu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = ((uint64_t)rv32->r[RS1] * (uint64_t)rv32->r[RS2]) >> 32;
        break;
      case 0x4: /* div */ {
        trace("div %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        int32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = 0xFFFFFFFF;
        else if (dividend == 0x80000000 && divisor == -1)
          rv32->r[RD] = dividend; /* overflow */
        else
          rv32->r[RD] = dividend / divisor;
        break;
      }
      case 0x5: /* divu */ {
        trace("divu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        uint32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = 0xFFFFFFFF;
        else
          rv32->r[RD] = dividend / divisor;
        break;
      }
      case 0x6: /* rem */ {
        trace("rem %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        int32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = dividend;
        else if (dividend == 0x80000000 && divisor == -1)
          rv32->r[RD] = 0; /* overflow) */
        else
          rv32->r[RD] = dividend % divisor;        
        break;
      }
      case 0x7: /* remu */ {
        trace("remu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        uint32_t dividend = rv32->r[RS1], divisor = rv32->r[RS2];
        if (divisor == 0)
          rv32->r[RD] = dividend;
        else
          rv32->r[RD] = dividend % divisor;
        break;
      }
      default:
        trace("invalid instruction\n");
        rv32->status = RV32_INVALID_INSTRUCTION;
        return;
        break;
      }
    } else {
      switch (funct3) {
      case 0x0:
        if (funct7 == 0x00) /* add */ {
          trace("add %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
          rv32->r[RD] = rv32->r[RS1] + rv32->r[RS2];
        }
        else if (funct7 == 0x20) /* sub */ {
          trace("sub %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
          rv32->r[RD] = rv32->r[RS1] - rv32->r[RS2];
        }
        else {
          rv32->status = RV32_INVALID_INSTRUCTION;
          return;
        }
        break;
      case 0x4: /* xor */
        trace("xor %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = rv32->r[RS1] ^ rv32->r[RS2];
        break;
      case 0x6: /* or */
        trace("or %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = rv32->r[RS1] | rv32->r[RS2];
        break;
      case 0x7: /* and */
        trace("and %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = rv32->r[RS1] & rv32->r[RS2];
        break;
      case 0x1: /* sll */
        trace("sll %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = rv32->r[RS1] << (rv32->r[RS2] & 0x1f);
        break;
      case 0x5:
        if (funct7 == 0x0) /* srl */ {
          trace("srl %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
          rv32->r[RD] = rv32->r[RS1] >> (rv32->r[RS2] & 0x1f);
        }
        else if (funct7 == 0x20) /* sra */ {
          trace("sra %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
          rv32->r[RD] = (int32_t)rv32->r[RS1] >> (rv32->r[RS2] & 0x1f);
        }
        else {
          trace("invalid instruction\n");
          rv32->status = RV32_INVALID_INSTRUCTION;
          return;
        }
        break;
      case 0x2: /* slt */
        trace("slt %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = (int32_t)rv32->r[RS1] < (int32_t)rv32->r[RS2] ? 1 : 0;
        break;
      case 0x3: /* sltu */
        trace("sltu %s, %s, %s\n", rname[RD], rname[RS1], rname[RS2]);
        rv32->r[RD] = rv32->r[RS1] < rv32->r[RS2] ? 1 : 0;
        break;
      default: {
        trace("invalid instruction\n");
        rv32->status = RV32_INVALID_INSTRUCTION;
        return;
      }
      }
    }
    rv32->pc += 4;
    break;

  case 0x13:
    switch (funct3) {
    case 0x00: /* addi */
      trace("addi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      rv32->r[RD] = rv32->r[RS1] + SEXT_IMM_I;
      break;
    case 0x4: /* xori */
      trace("xori %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      rv32->r[RD] = rv32->r[RS1] ^ SEXT_IMM_I;
      break;
    case 0x6: /* ori */
      trace("ori %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      rv32->r[RD] = rv32->r[RS1] | SEXT_IMM_I;
      trace("ori %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      break;
    case 0x7: /* andi */
      trace("andi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      rv32->r[RD] = rv32->r[RS1] & SEXT_IMM_I;
      break;
    case 0x1: /* slli */
      trace("slli %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
      rv32->r[RD] = rv32->r[RS1] << (IMM_I & 0x1f);
      break;
    case 0x5:
      if (funct7 == 0x00) { /* srli */
        trace("srli %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
        rv32->r[RD] = rv32->r[RS1] >> (IMM_I & 0x1f);
      } else if (funct7 == 0x20) { /* srai */
        trace("srai %s, %s, %u\n", rname[RD], rname[RS1], IMM_I & 0x1f);
        rv32->r[RD] = (int32_t)rv32->r[RS1] >> (IMM_I & 0x1f);
      } else {
        trace("invalid instruction\n");
        rv32->status = RV32_INVALID_INSTRUCTION;
        return;
      }
      break;
    case 0x2: /* slti */
      trace("andi %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
      rv32->r[RD] = (int32_t)rv32->r[RS1] < SEXT_IMM_I ? 1 : 0;
      break;
    case 0x3: /* sltiu */
      trace("sltiu %s, %s, %u\n", rname[RD], rname[RS1], IMM_I);
      rv32->r[RD] = rv32->r[RS1] < IMM_I ? 1 : 0;
      break;
    default:
      trace("invalid instruction\n");
      rv32->status = RV32_INVALID_INSTRUCTION;
      return;
    }
    rv32->pc += 4;
    break;

  case 0x3:
    addr = rv32->r[RS1] + SEXT_IMM_I;
    switch (funct3) {
    case 0x0: /* lb */
      trace("lb %s, %d(%s)\t0x%08x\n", rname[RD], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size) {
        if(mmio_load8(addr, &tmp8) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
        rv32->r[RD] = SEXT(tmp8, 8);
      } else {
        rv32->r[RD] = SEXT(LOAD8(addr), 8);
      }
      break;
    case 0x1: /* lh */
      trace("lh %s, %d(%s)\t0x%08x\n", rname[RD], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size - 1) {
        if(mmio_load16(addr, &tmp16) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
        rv32->r[RD] = SEXT(tmp16, 16);
      } else {
        rv32->r[RD] = SEXT(LOAD16(addr), 16);
      }
      break;
    case 0x2: /* lw */
      trace("lw %s, %d(%s)\t0x%08x\n", rname[RD], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size - 3) {
        if(mmio_load32(addr, &tmp32) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
        rv32->r[RD] = tmp32;
      } else {
        rv32->r[RD] = LOAD32(addr);
      }
      break;
    case 0x4: /* lbu */
      trace("lbu %s, %d(%s)\n", rname[RD], SEXT_IMM_I, rname[RS1]);
      if (addr >= rv32->mem_size) {
        if(mmio_load8(addr, &tmp8) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
        rv32->r[RD] = tmp8;
      } else {
        rv32->r[RD] = LOAD8(addr);
      }
      break;
    case 0x5: /* lhu */
      trace("lhu %s, %d(%s)\n", rname[RD], SEXT_IMM_I, rname[RS1]);
      if (addr >= rv32->mem_size - 1) {
        if(mmio_load16(addr, &tmp16) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
        rv32->r[RD] = tmp16;
      } else {
        rv32->r[RD] = LOAD16(addr);
      }
      break;
    default:
      trace("invalid instruction\n");
      rv32->status = RV32_INVALID_INSTRUCTION;
      return;
    }
    rv32->pc += 4;
    break;

  case 0x23:
    addr = rv32->r[RS1] + SEXT_IMM_S;
    switch (funct3) {
    case 0x0: /* sb */
      trace("sb %s, %d(%s)\t0x%08x\n", rname[RS2], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size) {
        if(mmio_store8(addr, rv32->r[RS2] & 0xff) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
      } else {
        STORE8(addr, rv32->r[RS2] & 0xff);
      }
      break;
    case 0x1: /* sh */
      trace("sh %s, %d(%s)\t0x%08x\n", rname[RS2], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size) {
        if(mmio_store16(addr, rv32->r[RS2] & 0xffff) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
      } else {
        STORE16(addr, rv32->r[RS2] & 0xffff);
      }
      break;
    case 0x2: /* sw */
      trace("sw %s, %d(%s)\t0x%08x\n", rname[RS2], SEXT_IMM_I, rname[RS1], addr);
      if (addr >= rv32->mem_size - 3) {
        if(mmio_store32(addr, rv32->r[RS2]) != RV32_MMIO_OK) {
          rv32->status = RV32_INVALID_MEMORY_ACCESS;
          return;
        }
      } else {
        STORE32(addr, rv32->r[RS2]);
      }
      break;
    default:
      trace("invalid instruction\n");
      rv32->status = RV32_INVALID_INSTRUCTION;
      return;
    }
    rv32->pc += 4;
    break;

  case 0x63:
    switch (funct3) {
    case 0x0: /* beq */
      trace("beq %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      if (rv32->r[RS1] == rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
      break;
    case 0x1: /* bne */
      trace("bne %s, %s, %d\n", rname[RS1], rname[RS2], SEXT_IMM_B);
      if (rv32->r[RS1] != rv32->r[RS2])
        rv32->pc += SEXT_IMM_B;
      else
        rv32->pc += 4;
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
      trace("invalid instruction\n");
      rv32->status = RV32_INVALID_INSTRUCTION;
      return;
    }
    break;

  case 0x6f: /* jal */
    trace("jal %s, %d\n", rname[RD], SEXT_IMM_J);
    rv32->r[RD] = rv32->pc + 4;
    rv32->pc += SEXT_IMM_J;
    break;

  case 0x67: /* jalr */
    trace("jalr %s, %s, %d\n", rname[RD], rname[RS1], SEXT_IMM_I);
    rv32->r[RD] = rv32->pc + 4;
    rv32->pc = rv32->r[RS1] + SEXT_IMM_I;
    break;

  case 0x37: /* lui */
    trace("lui %s, %d\n", rname[RD], SEXT_IMM_U);
    rv32->r[RD] = SEXT_IMM_U << 12;
    rv32->pc += 4;
    break;

  case 0x17: /* auipc */
    trace("auipc %s, %d\n", rname[RD], SEXT_IMM_U);
    rv32->r[RD] = rv32->pc + (SEXT_IMM_U << 12);
    rv32->pc += 4;
    break;

  case 0x73: /* SYSTEM */
    if(funct3 == 0) {
      switch (IMM_I) {
      case 0x0: /* ecall */
        trace("ecall %d\n", rv32->r[17]);
        ecall(rv32);
        if(rv32->status != RV32_RUNNING)
          return;
        rv32->pc += 4;
        break;
      case 0x1: /* ebreak */
        trace("ebreak\n");
        rv32->status = RV32_EBREAK;
        rv32->pc += 4;
        return;
      case 0x302: /* mret */
        rv32->pc = rv32->mepc;
        CSR_SET_BIT(rv32->mstatus, CSR_MSTATUS_MIE, CSR_GET_BIT(rv32->mstatus, CSR_MSTATUS_MPIE));
        break;
      case 0x105: /* wfi */
        rv32->wfi = 1;
        rv32->pc += 4;
        break;
      default:
        trace("invalid instruction\n");
        rv32->status = RV32_INVALID_INSTRUCTION;
        return;
      }
    } else if(funct3 & 0x3) { /* Zicsr function */
      uint32_t r_val = rv32_read_csr(rv32, IMM_I);
      uint32_t w_val = 0;
      uint32_t imm = RS1;
      uint32_t rs1 = rv32->r[RS1];
      switch(funct3) {
      case 0x1: /* csrrw : atomic read/write  */
        w_val = rs1;
        break;
      case 0x2: /* csrrs : atomic read and set bit */
        w_val = r_val | rs1;
        break;
      case 0x3: /* csrrc : atomic read and clear bit */
        w_val = r_val & ~rs1;
        break;
      case 0x5: /* csrrwi : immediate version of csrrw */
        w_val = imm;
        break;
      case 0x6: /* csrrsi : immediate version of csrrs */
        w_val = r_val | imm;
        break;
      case 0x7: /* csrrci : immediate version of csrrc */
        w_val = r_val & ~imm;
        break;
      default:
        trace("invalid instruction\n");
        rv32->status = RV32_INVALID_INSTRUCTION;
        return;
      }
      rv32_write_csr(rv32, IMM_I, w_val);
      rv32->r[RD] = r_val;
      rv32->pc += 4;
    } else {
      trace("invalid instruction\n");
      rv32->status = RV32_INVALID_INSTRUCTION;
      return;
    }
    break;
  default:
    trace("invalid opcode\n");
    rv32->status = RV32_INVALID_INSTRUCTION;
    return ;
  }
}

int rv32_set_breakpoint(RV32 *rv32, uint32_t addr) {
  int i;
  for(i = 0; i < 8; i++) {
    if((rv32->bp_mask & (1 << i)) == 0) {
      rv32->bp_mask |= (1 << i);
      rv32->bp[i] = addr;
      return 1;
    }
  }
  return 0;
}

int rv32_clear_breakpoint(RV32 *rv32, uint32_t addr) {
  int i;
  for(i = 0; i < 8; i++) {
    if((rv32->bp_mask & (1 << i)) && rv32->bp[i] == addr) {
      rv32->bp_mask &= ~(1<<i);
      rv32->bp[i] = 0;
      return 1;
    }
  }
  return 0;
}

#endif /* RV32_IMPLEMENTATION */
#endif /* INCLUDE_RV32_H */