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

float my_sqrt(float x) {
  print_int(x);
  float guess = 1.0f;
  while(guess * guess - x > 0.001)
    guess = (guess + x / guess) / 2.0f;
  return guess;
}

float my_abs(int x, int y) {
  return x * x + y * y;
}

static const unsigned int max_iter = 80;

unsigned int mandelbrot(float cx, float cy) {

  float zx = 0, zy = 0;
  unsigned int i;
  for(i = 0; my_abs(zx, zy) <= 4.0f && i < max_iter; i++) {
    zx = zx * zx - zy * zy + cx;
    zy = 2 * zx * zy + cy;
  }
  return i;
}

unsigned short rgb565(unsigned char r, unsigned char g, unsigned char b) {
  r >>= 2;
  g >>= 1;
  b >>= 2;
  return r << 11 | g << 5 | b;
}

__attribute__((noinline)) int main()
{
  unsigned int w = get_screen_width(), h = get_screen_height();
  print_int(w);
  print_int(h);
  volatile unsigned short *pixels = (volatile unsigned short*)0x80000000;

  const int re_start = -2, re_end = 1, im_start = -1, im_end = 1;
  float cx, cy;

  for(unsigned int y = 0; y < h; y++) {
    for(unsigned int x = 0; x < w; x++) {
      cx = re_start + ((float)x / w) * (re_end - re_start);
      cy = im_start + ((float)y / h) * (im_end - im_start);
      unsigned m = mandelbrot(cx, cy);
      unsigned char color = 255 - m * 255 / max_iter;
    pixels[y * w + x] = rgb565(color, color, color);
    }
    render();
  }
  render();
  return 0;
}
