#include <stdio.h>

int main(int argc, char *argv[])
{
  int c;
  while((c = fgetc(stdin)) != EOF) {
    if(c == '#') {
      fgetc(stdin);
      fgetc(stdin);
      printf("+$#00");
      fflush(stdout);
    }
  }
  return 0;
}
