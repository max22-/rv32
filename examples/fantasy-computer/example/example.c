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

__attribute__((noinline)) int main()
{
  unsigned int w = get_screen_width(), h = get_screen_height();
  print_int(w);
  print_int(h);
  volatile unsigned short *pixels = (volatile unsigned short*)0x80000000;
  for(unsigned int i = 0; i < h; i++)
    pixels[i * w + i] = 0x1f << 11;
  render();
  return 0;
}
