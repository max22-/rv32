
asm("j _start");

void print_int(int x)
{
  asm("li a7, 1");
  asm("ecall");
}

void quit(int code)
{
  asm("li a7, 93");
  asm("ecall");
}

int fib(int n)
{
  if(n == 0) return 0;
  if(n == 1) return 1;
  return fib(n-1) + fib(n-2);
}

void _start()
{
  int x = 0, y;
  x++;
  y = x * 2 + 1;
  print_int(y);
  print_int(fib(30));
  x = 45;
  y = 53;
  print_int(x*y);
  x = 15;
  y = 5;
  print_int(x/y);
  quit(0);
}
