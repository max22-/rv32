#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RSP_BUFFER_SIZE 1024
#define FATAL(...) \
  do { \
    fprintf(stderr, "error: "); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(1); \
  } while(0)

enum State {WAIT_START, PACKET_DATA, CHECKSUM_1, CHECKSUM_2, WAIT_ACK};

typedef struct {
  char buffer[RSP_BUFFER_SIZE];
  int ptr;
  uint8_t checksum;
} rsp_packet_t;

uint8_t hex2int(uint8_t h) {
  if(h >= '0' && h <= '9') return h - '0';
  if(h >= 'a' && h <= 'f') return h - 'a' + 10;
  if(h >= 'A' && h <= 'F') return h - 'A' + 10;
  FATAL("invalid hex digit");
}

uint8_t int2hex(uint8_t i) {
  const char digits[] = "0123456789abcdef";
  if(i > 0xf) FATAL("can't convert %d to hex digit", i);
  return digits[i];
}

void packet_begin(rsp_packet_t *p) {
  p->buffer[0] = '$';
  p->ptr = 1;
  p->checksum = 0;
}

void packet_append_byte_raw(rsp_packet_t *p, uint8_t b) {
  if(p->ptr < RSP_BUFFER_SIZE) {
    p->buffer[p->ptr++] = b;
    p->checksum += b;
  } else FATAL("output buffer too small");
}

void packet_append(rsp_packet_t *p, const char* data) {
  while(*data) {
    if(p->ptr < RSP_BUFFER_SIZE) {
      uint8_t c = *data++;
      if(c == '#' || c == '$' || c == '}') {
        packet_append_byte_raw(p, '}');
        c ^= 0x20;
      }
      packet_append_byte_raw(p, c);
    } else FATAL("output buffer too small");
  }
}

void packet_append_u8(rsp_packet_t *p,  uint8_t b) {
  packet_append_byte_raw(p, int2hex((b >> 4) & 0xf));
  packet_append_byte_raw(p, int2hex(b & 0xf));
}

void packet_append_u32(rsp_packet_t *p, uint32_t w) {
  for(int i = 0; i < 32; i += 8)
    packet_append_u8(p, (w >> i) & 0xff);
}

void packet_end(rsp_packet_t *p) {
  uint8_t saved_checksum = p->checksum;
  packet_append_byte_raw(p, '#');
  packet_append_u8(p, saved_checksum);
  packet_append_byte_raw(p, 0);
}

void packet_send(rsp_packet_t *p) {
  printf("%s", p->buffer);
  fflush(stdout);
}

void packet_quick_send(const char* data) {
  rsp_packet_t p;
  packet_begin(&p);
  packet_append(&p, data);
  packet_end(&p);
  packet_send(&p);
}

#define len(x) (sizeof(x) - 1) /* For const char arrays only */
#define isprefix(s1, s2) (size >= len(s1) && !strncmp(s1, (const char*)s2, len(s1)))
void handle_packet(uint8_t *buffer, size_t size) {
  const char 
    qSupported[] = "qSupported",
    qAttached[] = "qAttached";
  if(isprefix(qSupported, buffer))
    packet_quick_send("hwbreak+");
  else if(isprefix("?", buffer))
    packet_quick_send("S05");
  else if(isprefix(qAttached, buffer))
    packet_quick_send("1");
  else if(isprefix("g", buffer)) {
    rsp_packet_t p;
    packet_begin(&p);
    for(int i = 0; i < 33; i++)
      packet_append_u32(&p, i);
    packet_end(&p);
    packet_send(&p);
  }
  else
    printf("$#00");
}

void rsp_handle_byte(char c) {
  static uint8_t in_buffer[RSP_BUFFER_SIZE], sum1, sum2;
  static size_t iptr;
  static enum State state = WAIT_START;

  switch(state) {
    case WAIT_START:
      iptr = 0;
      sum1 = sum2 = 0;
      if(c == '$') state = PACKET_DATA;
      break;
    case PACKET_DATA:
      if(c == '#') state = CHECKSUM_1;
      else if(iptr < RSP_BUFFER_SIZE) {
        in_buffer[iptr++] = c;
        sum1 += c;
        #warning TODO: handle escaped characters
      } else FATAL("input buffer too small");
      break;
    case CHECKSUM_1:
      sum2 = hex2int(c) << 4;
      state = CHECKSUM_2;
      break;
    case CHECKSUM_2:
      sum2 |= hex2int(c);
      if(sum1 == sum2) {
        printf("+");
        handle_packet(in_buffer, iptr);
      } else printf("-");
      fflush(stdout);
      state = WAIT_ACK;
      break;
    case WAIT_ACK:
      if(c == '+')
        state = WAIT_START;
      else
        FATAL("gdb didn't acknowledge last packet"); /* TODO: implement re-send */
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
