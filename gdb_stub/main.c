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

char rsp_out_buffer[RSP_BUFFER_SIZE];
int optr;
uint8_t out_checksum;

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

void packet_begin() {
  rsp_out_buffer[0] = '$';
  optr = 1;
  out_checksum = 0;
}

void packet_append_byte_raw(uint8_t b) {
  if(optr < RSP_BUFFER_SIZE) {
    rsp_out_buffer[optr++] = b;
    out_checksum += b;
  } else FATAL("output buffer too small");
}

void packet_append(const char* data) {
  while(*data) {
    if(optr < RSP_BUFFER_SIZE) {
      uint8_t c = *data++;
      if(c == '#' || c == '$' || c == '}') {
        packet_append_byte_raw('}');
        c ^= 0x20;
      }
      packet_append_byte_raw(c);
    } else FATAL("output buffer too small");
  }
}

void packet_append_u8(uint8_t b) {
  packet_append_byte_raw(int2hex((b >> 4) & 0xf));
  packet_append_byte_raw(int2hex(b & 0xf));
}

void packet_append_u32(uint32_t w) {
  for(int i = 0; i < 32; i += 8)
    packet_append_u8((w >> i) & 0xff);
}

void packet_end() {
  uint8_t saved_checksum = out_checksum;
  packet_append_byte_raw('#');
  packet_append_u8(saved_checksum);
  packet_append_byte_raw(0);
}

void packet_send() {
  printf("%s", rsp_out_buffer);
  fflush(stdout);
}

void packet_quick_send(const char* data) {
  packet_begin();
  packet_append(data);
  packet_end();
  packet_send();
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
    packet_begin();
    for(int i = 0; i < 33; i++)
      packet_append_u32(i);
    packet_end();
    packet_send();
  }
  else
    printf("$#00");
}

int main(int argc, char *argv[])
{
  int c;
  uint8_t in_buffer[RSP_BUFFER_SIZE], sum1, sum2;
  size_t iptr;
  enum State state = WAIT_START;

  while((c = fgetc(stdin)) != EOF) {
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

  return 0;
}
