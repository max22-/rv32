void print_int(int x)
{
  asm("li a7, 2");
  asm("ecall");
}

int fib(int n)
{
  int i, a = 0, b = 1, c;
  if(n == 0) return a;
  if(n == 1) return b;
  for(i = 2; i <= n; i++) {
    c = a + b;
    a = b;
    b = c;
  }
  return c;
}

int main()
{
  int x = 0, y;
  x++;
  y = x * 2 + 1;
  print_int(y);
  print_int(fib(45));
  x = 45;
  y = 53;
  print_int(x*y);
  x = 15;
  y = 5;
  print_int(x/y);
  return 45;
}
