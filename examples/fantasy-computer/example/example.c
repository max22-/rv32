void print_int(int x)
{
  register int a0 asm("a0") = x;
  asm (
    "li a7, 2\n"
    "ecall"
    :
    : "r"(a0)
    : "a7"
  );
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

double my_sqrt(double x) {
  print_int(x);
  double guess = 1.0f;
  while(guess * guess - x > 0.001)
    guess = (guess + x / guess) / 2.0f;
  return guess;
}

double my_abs(double x, double y) {
  return x * x + y * y;
}

static const unsigned int max_iter = 80;

unsigned int mandelbrot(double cx, double cy) {

  double zx = 0, zy = 0;
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

void interrupt_handler() {
  print_int(42);
  asm("li t0, 0x800");
  asm("csrrc x0, mip, t0");
  asm("mret");
}

__attribute__((noinline)) int main()
{
  asm("csrw mtvec, %0" :: "r"(interrupt_handler));
  asm("li t0, 0x800");
  asm("csrrs x0, mie, t0");
  asm("csrrs x0, mstatus, 0x8");
  unsigned int w = get_screen_width(), h = get_screen_height();
  print_int(w);
  print_int(h);
  volatile unsigned short *pixels = (volatile unsigned short*)0x80000000;

  const double re_start = -2, re_end = 1, im_start = -1, im_end = 1;
  double cx, cy;

  for(unsigned int y = 0; y < h; y++) {
    print_int(y);
    for(unsigned int x = 0; x < w; x++) {
      if(x == 0 && y == h / 2) { asm volatile ("wfi"); }
      cx = re_start + ((double)x / w) * (re_end - re_start);
      cy = im_start + ((double)y / h) * (im_end - im_start);
      unsigned int m = mandelbrot(cx, cy);
      unsigned char color = 255 - (double)m * 255 / max_iter;
    pixels[y * w + x] = rgb565(color, color, color);
    }
    render();
  }
  render();
  return 0;
}
