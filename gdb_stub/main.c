#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#define RSP_IMPLEMENTATION
#define RV32_IMPLEMENTATION
#include "rsp.h"

rv32_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr==0x80000000) { 
    printf("%c", val); 
    return RV32_OK;
  }
  return RV32_INVALID_MEMORY_ACCESS;
}
rv32_result_t mmio_store16(uint32_t addr, uint16_t val) { return RV32_INVALID_MEMORY_ACCESS; }
rv32_result_t mmio_store32(uint32_t addr, uint32_t val) { return RV32_INVALID_MEMORY_ACCESS; }

void ecall(RV32 *rv32) { 
  if(rv32->r[REG_A7] == 93) rv32->running = 0;
}

int
input_available (int fd)
{
  fd_set set;
  struct timeval timeout;

  FD_ZERO (&set);
  FD_SET (fd, &set);

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  return select (FD_SETSIZE, &set, NULL, NULL, &timeout) == 1;
}


int main(int argc, char *argv[])
{
  RV32 *rv32;
  rv32_result_t res;
  int c = 0;

  rv32 = rv32_new(0x10000, calloc);
  if (!rv32) {
    fprintf(stderr, "Not enough memory\n");
    return 1;
  }
/* TODO: add program reset + bp clear */
  while(1) {
    if(input_available(STDIN_FILENO) || rv32->running == 0) {
      c = fgetc(stdin);
      if(c == EOF)
        break;
      rsp_handle_byte(rv32, c);
    }
    if(rv32->running) {
      res = rv32_cycle(rv32);
      switch(res) {
        case RV32_EBREAK:
        case RV32_BREAKPOINT:
        case RV32_PAUSED:
          rsp_report_signal(RSP_SIGTRAP);
          break;
        case RV32_INVALID_MEMORY_ACCESS:
          #warning quick hack
          rv32->running = 0;
          rsp_report_signal(RSP_SIGSEGV);
          break;
        case RV32_OK:
          #warning TODO: do that better ?
          if(rv32->running == 0)
            rsp_report_signal(RSP_SIGTRAP);
          break;
        default:
          fprintf(stderr, "res=%d\n", res);
      }
      
      /*else if(res != RV32_OK)*/
    }
  }
  return 0;
}
