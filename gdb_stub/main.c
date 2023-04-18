#include <stdio.h>
#define RSP_IMPLEMENTATION
#include "rsp.h"

int main(int argc, char *argv[])
{
  int c;

  while((c = fgetc(stdin)) != EOF)
    rsp_handle_byte(c);

  return 0;
}
