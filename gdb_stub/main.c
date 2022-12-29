#include <stdio.h>
#include <stdint.h>

enum State {WAIT_START, PACKET_DATA, CHECKSUM_1, CHECKSUM_2};

void send_packet(uint8_t *data, size_t len)
{
  printf("$");
  uint8_t sum = 0;
  for(int i = 0; i < len; i++) {
    uint8_t c = data[i];
    sum += c;
    if(c == '#' || c == '$' || c == '}') printf("}%c", c ^ 0x20);
    else printf("%c", c);
  }
  printf("#%02x", c);
  fflush(stdout);
}

int main(int argc, char *argv[])
{
  int c;
  enum State state = WAIT_START;
  while((c = fgetc(stdin)) != EOF) {
    switch(state) {
    case WAIT_START:
      if(c == '$') state = PACKET_DATA;
      break;
    case PACKET_DATA:
      if(c == '#') state = CHECKSUM_1;
      break;
    case CHECKSUM_1:
      state = CHECKSUM_2;
      break;
    case CHECKSUM_2:
      state = WAIT_START;
      printf("+$#00");
      fflush(stdout);
      break;
    }
  }
  return 0;
}
