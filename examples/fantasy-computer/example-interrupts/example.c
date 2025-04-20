void print_int(int x)
{
  register int a0 asm("a0") = x;
  asm (
    "li a7, 2\n"
    "ecall"
    :
    : "r"(a0)
    :
  );
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
  while(1);
  return 0;
}
