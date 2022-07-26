#include <stdint.h>

typedef struct {
  uint32_t mem_size;
  uint8_t halted;
  uint32_t r[32], pc;
  uint8_t mem[1];
} RV32;

typedef enum {RV32_OK, RV32_INVALID_OPCODE, RV32_INVALID_INSTRUCTION, RV32_INVALID_MEMORY_ACCESS, RV32_DIV_BY_ZERO} rv32_result_t;

RV32 *rv32_new(uint32_t mem_size);
void rv32_free(RV32 *rv32);
void rv32_dump_registers(RV32 *rv32);
rv32_result_t rv32_cycle(RV32 *rv32);
