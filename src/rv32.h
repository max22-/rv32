#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t mem_size;
  uint8_t halted;
  uint32_t r[32], pc;
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

typedef enum {
  RV32_OK,
  RV32_INVALID_OPCODE,
  RV32_INVALID_INSTRUCTION,
  RV32_INVALID_MEMORY_ACCESS
} rv32_result_t;

RV32 *rv32_new(uint32_t mem_size, void *(*calloc_func)(size_t, size_t));
rv32_result_t rv32_cycle(RV32 *rv32);
