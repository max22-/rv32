#include <stdint.h>

typedef struct {
  uint32_t mem_size;
  uint8_t halted;
  uint32_t r[32], pc;
  uint8_t mem[1];
} RV32;

RV32 *rv32_new(uint32_t mem_size);
void rv32_free(RV32 *rv32);
void rv32_dump_registers(RV32 *rv32);
void rv32_cycle(RV32 *rv32);
