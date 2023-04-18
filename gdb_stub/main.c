#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RSP_BUFFER_SIZE 1024
#define RSP_FATAL(...) \
  do { \
    fprintf(stderr, "error: "); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(1); \
  } while(0)

enum rsp_state {RSP_WAIT_START, RSP_PACKET_DATA, RSP_CHECKSUM_1, RSP_CHECKSUM_2, RSP_WAIT_ACK};

typedef struct {
  char buffer[RSP_BUFFER_SIZE];
  int ptr;
  uint8_t checksum;
} rsp_packet_t;

uint8_t rsp_hex2int(uint8_t h) {
  if(h >= '0' && h <= '9') return h - '0';
  if(h >= 'a' && h <= 'f') return h - 'a' + 10;
  if(h >= 'A' && h <= 'F') return h - 'A' + 10;
  RSP_FATAL("invalid hex digit");
}

uint8_t rsp_int2hex(uint8_t i) {
  const char digits[] = "0123456789abcdef";
  if(i > 0xf) RSP_FATAL("can't convert %d to hex digit", i);
  return digits[i];
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
  rsp_packet_append_byte_raw(p, rsp_int2hex((b >> 4) & 0xf));
  rsp_packet_append_byte_raw(p, rsp_int2hex(b & 0xf));
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

void rsp_packet_send(rsp_packet_t *p) {
  printf("%s", p->buffer);
  fflush(stdout);
}

void rsp_packet_quick_send(const char* data) {
  rsp_packet_t p;
  rsp_packet_begin(&p);
  rsp_packet_append(&p, data);
  rsp_packet_end(&p);
  rsp_packet_send(&p);
}

#define len(x) (sizeof(x) - 1) /* For const char arrays only */
#define isprefix(s1, s2) (size >= len(s1) && !strncmp(s1, (const char*)s2, len(s1)))
void rsp_handle_packet(uint8_t *buffer, size_t size) {
  const char 
    qSupported[] = "qSupported",
    qAttached[] = "qAttached";
  if(isprefix(qSupported, buffer))
    rsp_packet_quick_send("hwbreak+");
  else if(isprefix("?", buffer))
    rsp_packet_quick_send("S05");
  else if(isprefix(qAttached, buffer))
    rsp_packet_quick_send("1");
  else if(isprefix("g", buffer)) {
    rsp_packet_t p;
    rsp_packet_begin(&p);
    for(int i = 0; i < 33; i++)
      rsp_packet_append_u32(&p, i);
    rsp_packet_end(&p);
    rsp_packet_send(&p);
  }
  else
    printf("$#00");
}

void rsp_handle_byte(char c) {
  static uint8_t in_buffer[RSP_BUFFER_SIZE], sum1, sum2;
  static size_t iptr;
  static enum rsp_state state = RSP_WAIT_START;

  switch(state) {
    case RSP_WAIT_START:
      iptr = 0;
      sum1 = sum2 = 0;
      if(c == '$') state = RSP_PACKET_DATA;
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
      sum2 = rsp_hex2int(c) << 4;
      state = RSP_CHECKSUM_2;
      break;
    case RSP_CHECKSUM_2:
      sum2 |= rsp_hex2int(c);
      if(sum1 == sum2) {
        printf("+");
        rsp_handle_packet(in_buffer, iptr);
      } else printf("-");
      fflush(stdout);
      state = RSP_WAIT_ACK;
      break;
    case RSP_WAIT_ACK:
      if(c == '+')
        state = RSP_WAIT_START;
      else
        RSP_FATAL("gdb didn't acknowledge last packet"); /* TODO: implement re-send */
      break;
    }
}

int main(int argc, char *argv[])
{
  int c;

  while((c = fgetc(stdin)) != EOF)
    rsp_handle_byte(c);

  return 0;
}
