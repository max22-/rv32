#include <Arduino.h>
#define TRACE
#define LITTLE_ENDIAN_HOST
#define RSP_IMPLEMENTATION
#define RV32_IMPLEMENTATION
#define RSP_SEND(x) \
  do { \
    Serial1.printf("%s", x); \
    Serial1.flush(); \
  } while(0);

#define RSP_FATAL(msg) \
  do { \
    Serial.printf("%s:%d: error: %s\n", __FILE__, __LINE__, msg); \
    while(1) delay(1000); \
  } while(0)

extern "C" {
#include "rv32.h"
#include "rsp.h"
}

RV32 *rv32;

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr==0x80000000) { 
    Serial.printf("%c", val); 
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_MMIO_ERR; }

void ecall(RV32 *rv32) { 
  if(rv32->r[REG_A7] == 2) Serial.printf("%d\n", rv32->r[REG_A0]);
  if(rv32->r[REG_A7] == 93) rv32->status = RV32_HALTED;
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  rv32 = rv32_new(0x10000, calloc);
  if (!rv32) {
    Serial.println("Not enough memory\n");
    while(true) delay(1000);
  }
  rv32->status = RV32_BREAKPOINT;
  while(!Serial);
  Serial.printf("rv32 = 0x%x\n", rv32);
  Serial.printf("test1: 0x%x\n", *(uint32_t*)&rv32->mem[0]);
}

void loop() {
  int c;
  if(Serial1.available() > 0 || rv32->status != RV32_RUNNING) {
    while(Serial1.available() <= 0);
    c = Serial1.read();
    rsp_handle_byte(rv32, c);
  }
  if(rv32->status == RV32_RUNNING) {
    rv32_cycle(rv32);
    switch(rv32->status) {
      case RV32_RUNNING: /* do nothing */
        break;
      case RV32_INVALID_MEMORY_ACCESS:
        rsp_report_signal(RSP_SIGSEGV);
        break;
      case RV32_INVALID_INSTRUCTION:
        rsp_report_signal(RSP_SIGILL);
      default:
        rsp_report_signal(RSP_SIGTRAP);
    }
  }

}
