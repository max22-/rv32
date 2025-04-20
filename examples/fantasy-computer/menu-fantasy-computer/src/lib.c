#include "lib.h"

void print_int(int x)
{
  asm("li a7, 2");
  asm("ecall");
}

unsigned int get_screen_width() {
  volatile unsigned int res;
  asm("li a7, 3\n"
      "ecall\n" 
      "sw a0, %0"
      : "=rm" (res)
      :
      :);
  return res;
}

unsigned int get_screen_height() {
  volatile unsigned int res;
  asm("li a7, 4\n"
      "ecall\n" 
      "sw a0, %0"
      : "=rm" (res)
      :
      :);
  return res;
}

void render() {
  asm("li a7, 5");
  asm("ecall");
}

void draw_rectangle(int x, int y, int w, int h, short color) {
  asm("li a7, 6");
  asm("ecall");
}

void draw_char(unsigned short unicode, int x, int y) {
  asm("li a7, 7");
  asm("ecall");
}


unsigned short rgb565(unsigned char r, unsigned char g, unsigned char b) {
  r >>= 2;
  g >>= 1;
  b >>= 2;
  return r << 11 | g << 5 | b;
}