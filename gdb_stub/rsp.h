#ifndef INCLUDE_RSP_H
#define INCLUDE_RSP_H

#include "rv32.h"

enum rsp_signal {
  RSP_SIGINT = 2,
  RSP_SIGILL = 4,
  RSP_SIGTRAP = 5,
  RSP_SIGSEGV = 11
};

void rsp_report_signal(enum rsp_signal);
void rsp_handle_byte(RV32 *, char);


#if defined(RSP_IMPLEMENTATION)

#include <string.h>
#include <stdint.h>

#if !defined(RSP_SEND)
#include <stdio.h>
#define RSP_SEND(x) \
  do {\
    printf("%s", x); \
    fflush(stdout); \
  } while(0)
#endif

#if !defined(RSP_FATAL)
#include <stdio.h>
#include <stdlib.h>
#define RSP_FATAL(msg) \
  do { \
    fprintf(stderr, "%s:%d: error: %s\n", __FILE__, __LINE__, msg); \
    exit(1); \
  } while(0)
#endif

#define RSP_BUFFER_SIZE 1024

enum rsp_state {RSP_WAIT_START, RSP_PACKET_DATA, RSP_CHECKSUM_1, RSP_CHECKSUM_2, RSP_WAIT_ACK};
typedef enum {RSP_NO_PACKET_SENT, RSP_PACKET_SENT} rsp_handler_result_t;

typedef struct {
  char buffer[RSP_BUFFER_SIZE];
  int ptr;
  uint8_t checksum;
} rsp_packet_t;

int rsp_is_hex_digit(uint8_t h) {
  if(h >= '0' && h <= '9') return 1;
  if(h >= 'a' && h <= 'f') return 1;
  if(h >= 'A' && h <= 'F') return 1;
  return 0;
}

/* example: 'a' -> 0xa */
uint8_t rsp_from_hex_digit(uint8_t h) {
  if(h >= '0' && h <= '9') return h - '0';
  if(h >= 'a' && h <= 'f') return h - 'a' + 10;
  if(h >= 'A' && h <= 'F') return h - 'A' + 10;
  RSP_FATAL("invalid hex digit");
}

/* example: 0xa -> 'a' */
uint8_t rsp_to_hex_digit(uint8_t i) {
  const char digits[] = "0123456789abcdef";
  if(i > 0xf) RSP_FATAL("can't convert to hex digit");
  return digits[i];
}

/* example: "ab" -> 0xab */
uint8_t rsp_hex_to_u8(const uint8_t *h) {
  return (rsp_from_hex_digit(h[0]) << 4)
    | (rsp_from_hex_digit(h[1]));
}

/* example: "cdab3412" -> 0x1234abcd */
uint32_t rsp_hex_to_u32(const uint8_t *h) {
  uint32_t ret = 0;
  for(int i = 0; i < 4; i++) {
    ret |= rsp_hex_to_u8(h) << (i * 4);
    h += 2;
  }
  return ret;
}

void rsp_packet_begin(rsp_packet_t *p) {
  p->buffer[0] = '$';
  p->ptr = 1;
  p->checksum = 0;
}

void rsp_packet_append_byte_raw(rsp_packet_t *p, uint8_t b) {
  if(p->ptr < RSP_BUFFER_SIZE) {
    p->buffer[p->ptr++] = b;
    p->checksum += b;
  } else RSP_FATAL("output buffer too small");
}

void rsp_packet_append(rsp_packet_t *p, const char* data) {
  while(*data) {
    if(p->ptr < RSP_BUFFER_SIZE) {
      uint8_t c = *data++;
      if(c == '#' || c == '$' || c == '}') {
        rsp_packet_append_byte_raw(p, '}');
        c ^= 0x20;
      }
      rsp_packet_append_byte_raw(p, c);
    } else RSP_FATAL("output buffer too small");
  }
}

void rsp_packet_append_u8(rsp_packet_t *p,  uint8_t b) {
  rsp_packet_append_byte_raw(p, rsp_to_hex_digit((b >> 4) & 0xf));
  rsp_packet_append_byte_raw(p, rsp_to_hex_digit(b & 0xf));
}

void rsp_packet_append_u32(rsp_packet_t *p, uint32_t w) {
  for(int i = 0; i < 32; i += 8)
    rsp_packet_append_u8(p, (w >> i) & 0xff);
}

void rsp_packet_end(rsp_packet_t *p) {
  uint8_t saved_checksum = p->checksum;
  rsp_packet_append_byte_raw(p, '#');
  rsp_packet_append_u8(p, saved_checksum);
  rsp_packet_append_byte_raw(p, 0);
}

rsp_handler_result_t rsp_packet_send(rsp_packet_t *p) {
  RSP_SEND(p->buffer);
  return RSP_PACKET_SENT;
}

rsp_handler_result_t rsp_packet_quick_send(const char* data) {
  rsp_packet_t p;
  rsp_packet_begin(&p);
  rsp_packet_append(&p, data);
  rsp_packet_end(&p);
  rsp_packet_send(&p);
  return RSP_PACKET_SENT;
}

rsp_handler_result_t rsp_read_registers(RV32 *rv32) {
    rsp_packet_t p;
    rsp_packet_begin(&p);
    for(int i = 0; i < 32; i++)
        rsp_packet_append_u32(&p, rv32->r[i]);
    rsp_packet_append_u32(&p, rv32->pc);
    rsp_packet_end(&p);
    return rsp_packet_send(&p);
}

rsp_handler_result_t rsp_write_registers(RV32 *rv32, uint8_t *buffer, size_t size) {
    if(size != 265) /* 'G' + 33 registers * 8 nibbles */
        RSP_FATAL("received invalid number of registers");
    buffer++; size--; /* We discard the 'G' */
    for(int i = 0; i < 32; i++) {
      rv32->r[i] = rsp_hex_to_u32(buffer);
      buffer += 8; /* 8 hex digits per u32 numbers */
    }
    rv32->pc = rsp_hex_to_u32(buffer);
    return rsp_packet_quick_send("");
}

rsp_handler_result_t rsp_write_specific_register(RV32 *rv32, uint8_t *buffer, size_t size) {
  if(size != 12) /* 'P' + 2 hex digits + '=' + 8 hex digits = 12 bytes*/
    RSP_FATAL("invalid 'P' message from gdb");
  buffer++; /* We discard the 'P' */
  uint8_t reg = rsp_hex_to_u8(buffer);
  buffer += 3;
  uint32_t val = rsp_hex_to_u32(buffer);
  if(reg > 32) {
    RSP_FATAL("invalid register in P command");
  } else if(reg == 32) {
    rv32->pc = val;
  } else {
    rv32->r[reg] = val;
  }
  return rsp_packet_quick_send("OK");
}

rsp_handler_result_t rsp_read_memory(RV32 *rv32, uint8_t *buffer, size_t size) {
  rsp_packet_t p;
  uint32_t start = 0, chunk_size = 0;
  buffer++; size--;
  if(!rsp_is_hex_digit(buffer[0]))
    RSP_FATAL("invalid 'm' message from gdb");
  while(rsp_is_hex_digit(buffer[0]) && size) {
    start <<= 4;
    start |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(!size || buffer[0] != ',')
    RSP_FATAL("invalid 'm' message from gdb");
  buffer++; size--;
  while(rsp_is_hex_digit(buffer[0]) && size) {
    chunk_size <<= 4;
    chunk_size |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(size != 0)
    RSP_FATAL("invalid 'm' message from gdb");
  if(start >= rv32->mem_size || start + chunk_size >= rv32->mem_size) {
    return rsp_packet_quick_send("");
    //RSP_FATAL("invalid memory access");
  }
  rsp_packet_begin(&p);
  for(int i = 0; i < chunk_size; i++)
    rsp_packet_append_u8(&p, rv32->mem[start+i]);
  rsp_packet_end(&p);
  return rsp_packet_send(&p);
}

rsp_handler_result_t rsp_write_memory(RV32 *rv32, uint8_t *buffer, size_t size) {
  uint32_t start = 0, chunk_size = 0;

  buffer++; size--;
  if(!rsp_is_hex_digit(buffer[0]))
    RSP_FATAL("invalid 'M' message from gdb");
  while(rsp_is_hex_digit(buffer[0]) && size) {
    start <<= 4;
    start |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(!size || buffer[0] != ',')
    RSP_FATAL("invalid 'M' message from gdb");
  buffer++; size--;
  while(rsp_is_hex_digit(buffer[0]) && size) {
    chunk_size <<= 4;
    chunk_size |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(buffer[0] != ':')
    RSP_FATAL("invalid 'M' message from gdb");
  buffer++; size--;
  if(size != 2 * chunk_size)
    RSP_FATAL("invalid size in 'M' message from gdb");
  if(start >= rv32->mem_size || start + chunk_size >= rv32->mem_size) {
    return rsp_packet_quick_send("");
    //RSP_FATAL("invalid memory access");
  }
  for(int i = 0; i < chunk_size; i++) {
    rv32->mem[start + i] = rsp_hex_to_u8(buffer);
    buffer += 2;
  }
  return rsp_packet_quick_send("");
}

rsp_handler_result_t rsp_set_software_breakpoint(RV32* rv32, uint8_t *buffer, size_t size) {
  uint32_t addr = 0;
  buffer+=2; size -= 2;
  if(!size || buffer[0] != ',') RSP_FATAL("invalid 'Z0' message from GDB");
  buffer++; size--;
  while(rsp_is_hex_digit(buffer[0]) && size) {
    addr <<= 4;
    addr |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(buffer[0] != ',')
    RSP_FATAL("invalid 'Z0' message from gdb");
  if(rv32_set_breakpoint(rv32, addr))
    return rsp_packet_quick_send("OK");
  else
    return rsp_packet_quick_send("");
}

/* TODO: factorize set and clear */
rsp_handler_result_t rsp_clear_software_breakpoint(RV32* rv32, uint8_t *buffer, size_t size) {
  uint32_t addr = 0;
  buffer+=2; size -= 2;
  if(!size || buffer[0] != ',') RSP_FATAL("invalid 'Z0' message from GDB");
  buffer++; size--;
  while(rsp_is_hex_digit(buffer[0]) && size) {
    addr <<= 4;
    addr |= rsp_from_hex_digit(buffer[0]);
    buffer++; size--;
  }
  if(buffer[0] != ',')
    RSP_FATAL("invalid 'Z0' message from gdb");
  if(rv32_clear_breakpoint(rv32, addr))
    return rsp_packet_quick_send("OK");
  else
    return rsp_packet_quick_send("");
}

rsp_handler_result_t rsp_continue(RV32* rv32) {
  rv32_resume(rv32);
  return RSP_NO_PACKET_SENT;
}

rsp_handler_result_t rsp_detach(RV32* rv32) {
  rv32_clear_all_breakpoints(rv32);
  rv32_resume(rv32);
  return rsp_packet_quick_send("OK");
}

rsp_handler_result_t rsp_kill(RV32* rv32) {
  rv32_reset(rv32);
  return RSP_NO_PACKET_SENT;
}

#define len(x) (sizeof(x) - 1) /* For const char arrays only */
#define isprefix(s1, s2) (size >= len(s1) && !strncmp(s1, (const char*)s2, len(s1)))
rsp_handler_result_t rsp_handle_packet(RV32 *rv32, uint8_t *buffer, size_t size) {
  const char 
    qSupported[] = "qSupported",
    qAttached[] = "qAttached";
  if(isprefix(qSupported, buffer))
    return rsp_packet_quick_send("hwbreak+");
  else if(isprefix("?", buffer))
    return rsp_packet_quick_send("S05");
  else if(isprefix(qAttached, buffer))
    return rsp_packet_quick_send("1");
  else if(isprefix("g", buffer))
    return rsp_read_registers(rv32);
  else if(isprefix("G", buffer))
    return rsp_write_registers(rv32, buffer, size);
  else if(isprefix("P", buffer))
    return rsp_write_specific_register(rv32, buffer, size);
  else if(isprefix("m", buffer))
    return rsp_read_memory(rv32, buffer, size);
  else if(isprefix("M", buffer))
    return rsp_write_memory(rv32, buffer, size);
  else if(isprefix("Z0", buffer))
    return rsp_set_software_breakpoint(rv32, buffer, size);
  else if(isprefix("z0", buffer))
    return rsp_clear_software_breakpoint(rv32, buffer, size);
  else if(isprefix("c", buffer))
    return rsp_continue(rv32);
  else if(isprefix("D", buffer))
    return rsp_detach(rv32);
  else if(isprefix("k", buffer))
    return rsp_kill(rv32);
  else
    return rsp_packet_quick_send("");
}

void rsp_report_signal(enum rsp_signal signal) {
  switch(signal) { /* TODO: refactor that... */
    case RSP_SIGINT:
      rsp_packet_quick_send("S02");
      break;
    case RSP_SIGILL:
      rsp_packet_quick_send("S04");
      break;
    case RSP_SIGTRAP:
      rsp_packet_quick_send("S05");
      break;
    case RSP_SIGSEGV:
      rsp_packet_quick_send("S0b");
      break;
    default:
      RSP_FATAL("unknown signal");
  }
}

void rsp_handle_byte(RV32 *rv32, char c) {
  static uint8_t in_buffer[RSP_BUFFER_SIZE], sum1, sum2;
  static size_t iptr;
  static enum rsp_state state = RSP_WAIT_START;
  rsp_handler_result_t handler_result;

  switch(state) {
    case RSP_WAIT_START:
      iptr = 0;
      sum1 = sum2 = 0;
      if(c == '$') state = RSP_PACKET_DATA;
      else if(c == 3) {
        rv32->status = RV32_BREAKPOINT;
        rsp_report_signal(RSP_SIGINT);
      }
      break;
    case RSP_PACKET_DATA:
      if(c == '#') state = RSP_CHECKSUM_1;
      else if(iptr < RSP_BUFFER_SIZE) {
        in_buffer[iptr++] = c;
        sum1 += c;
        #warning TODO: handle escaped characters
      } else RSP_FATAL("input buffer too small");
      break;
    case RSP_CHECKSUM_1:
      sum2 = rsp_from_hex_digit(c) << 4;
      state = RSP_CHECKSUM_2;
      break;
    case RSP_CHECKSUM_2:
      sum2 |= rsp_from_hex_digit(c);
      if(sum1 == sum2) {
        RSP_SEND("+");
        handler_result = rsp_handle_packet(rv32, in_buffer, iptr);
      } else RSP_SEND("-");
      fflush(stdout);
      if(handler_result == RSP_PACKET_SENT)
        state = RSP_WAIT_ACK;
      else state = RSP_WAIT_START;
      break;
    case RSP_WAIT_ACK:
      if(c == '+')
        state = RSP_WAIT_START;
      else
        RSP_FATAL("gdb didn't acknowledge last packet"); /* TODO: implement re-send */
      break;
    }
}

#endif /* RSP_IMPLEMENTATION */
#endif /* INCLUDE_RSP_H */