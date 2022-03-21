#include <stdio.h>

int main(int argc, char** argv)
{
  int ix;
//  printf("argc: %d\n", argc);
  for (ix = 0; ix < argc; ++ix) {
    puts(argv[ix]);
  }
  return argc;
};
