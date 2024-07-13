#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;
TFT_eSprite sprite(&tft);

//#define TRACE
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

RV32 *rv32 = nullptr;
void *pixels = nullptr;

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t)) { 
    ((uint8_t*)pixels)[addr - 0x80000000] = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t) - (sizeof(uint32_t) - 1)) {
    *(uint32_t*)((uint8_t*)pixels + addr - 0x80000000) = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_MMIO_ERR; }

void ecall(RV32 *rv32) {
  switch (rv32->r[REG_A7]) {
  case 1:
    printf("%c", rv32->r[REG_A0]);
    break;
  case 2:
    printf("%d\n", rv32->r[REG_A0]);
    break;
  case 3: /* get screen width */
    rv32->r[REG_A0] = sprite.width();
    break;
  case 4:
    rv32->r[REG_A0] = sprite.height();
    break;
  case 5:
    sprite.pushSprite(0, 0);
    break;
  case 93: /* exit */
    rv32->status = RV32_HALTED;
    break;
  default:
    break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 4, 2);
  tft.begin();
  tft.setRotation(3);
  pixels = sprite.createSprite(tft.width(), tft.height());
  if(!pixels) {
    Serial.println("failed to create sprite");
    while(true) delay(1000);
  }
  sprite.fillScreen(TFT_BLACK);
  sprite.pushSprite(0, 0);
  const size_t ram_size = 0x10000;
  void *memory = calloc(RV32_NEEDED_MEMORY(ram_size), 1);
  if(!memory) {
    Serial.println("Failed to allocate memory\n");
    while(true) delay(1000);
  }
  rv32 = rv32_new(memory, ram_size);
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
    Serial.print((char)c);
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
