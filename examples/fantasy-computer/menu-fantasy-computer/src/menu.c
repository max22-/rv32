#include <lib.h>

__attribute__((noinline)) int main()
{
  unsigned int w = get_screen_width(), h = get_screen_height();
  print_int(w);
  print_int(h);
  volatile unsigned short *pixels = (volatile unsigned short*)0x80000000;

  draw_rectangle(0, 0, w, h, rgb565(0, 0, 0));
  draw_rectangle(0, 0, w, h, rgb565(0, 0, 255));
  //draw_rectangle(10, 10, 10, 10, rgb565(255, 0, 0));
  for(int i = 0; i < 26; i++)
    draw_char('a' + i, 10 + 10 * i, 100);

  render();
  return 0;
}
