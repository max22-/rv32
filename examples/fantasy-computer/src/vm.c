#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "vm.h"
#define RV32_IMPLEMENTATION
#include "rv32.h"
#include "shared.h"

const size_t mem_size = 0x10000; /* 64k */
static RV32 *rv32 = NULL;
static char vm_error[1024] = {0};
pthread_t thread;

#define VM_ERROR(...) snprintf(vm_error, sizeof(vm_error), __VA_ARGS__)


static void *vm_thread(void *args) {
    while(rv32->status == RV32_RUNNING) {
        rv32_cycle(rv32);
        switch(rv32->status) {
        case RV32_RUNNING:
            break;
        case RV32_HALTED:
            printf("cpu halted.\n");
            printf("exit status: %d\n", rv32->r[REG_A0]);
            break;
        case RV32_EBREAK:
            fprintf(stderr, "ebreak at pc=%08x\n", rv32->pc);
            break;
        default:
            fprintf(stderr, "Error %d at pc=%08x\n", rv32->status, rv32->pc);
            fprintf(stderr, "instr = %08x\n", *(uint32_t *)&rv32->mem[rv32->pc]);
        }
    }
    
    return NULL;
}

int vm_start(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if(!f) {
        VM_ERROR("failed to open %s", file_path);
        return 0;
    }
    rv32 = rv32_new(mem_size, calloc);
    if(!rv32) {
        fclose(f);
        VM_ERROR("failed to create VM (not enough memory)");
        return 0;
    }
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    if(file_size > mem_size) {
        free(rv32);
        fclose(f);
        VM_ERROR("program too big");
        return 0;
    }
    fseek(f, 0, SEEK_SET);
    size_t n = fread(rv32->mem, file_size, 1, f);
    fclose(f);
    if(n != 1) {
        free(rv32);
        VM_ERROR("failed to read program");
        return 0;
    }
    pthread_create(&thread, NULL, vm_thread, NULL);
    return 1;
}

const char *vm_get_error(void) {
    return vm_error;
}

/* syscalls, mmio *********************************************************** */

rv32_mmio_result_t mmio_load8(uint32_t addr, uint8_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load16(uint32_t addr, uint16_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_load32(uint32_t addr, uint32_t *ret) { return RV32_MMIO_ERR; }
rv32_mmio_result_t mmio_store8(uint32_t addr, uint8_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)) { 
    ((uint8_t*)pixels)[addr - 0x80000000] = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store16(uint32_t addr, uint16_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t) - (sizeof(uint16_t) - 1)) { 
    *(uint16_t*)((uint8_t*)pixels + addr - 0x80000000) = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}
rv32_mmio_result_t mmio_store32(uint32_t addr, uint32_t val) { 
  if(addr >= 0x80000000 && addr < 0x80000000 + SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t) - (sizeof(uint32_t) - 1)) { 
    *(uint32_t*)((uint8_t*)pixels + addr - 0x80000000) = val;
    return RV32_MMIO_OK;
  }
  return RV32_MMIO_ERR;
}

void ecall(RV32 *rv32) {
  switch (rv32->r[REG_A7]) {
  case 1:
    printf("%c", rv32->r[REG_A0]);
    break;
  case 2:
    printf("%d\n", rv32->r[REG_A0]);
    break;
  case 3: /* get screen width */
    rv32->r[REG_A0] = SCREEN_WIDTH;
    break;
  case 4:
    rv32->r[REG_A0] = SCREEN_HEIGHT;
    break;
  case 5:
    render = true;
    break;
  case 93: /* exit */
    rv32->status = RV32_HALTED;
    break;
  default:
    break;
  }
}
