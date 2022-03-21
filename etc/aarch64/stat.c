#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  struct stat sb;

  printf("sizeof: %ld\n", sizeof(struct stat));
  printf("offsetof: %ld\n", (void*) &sb.st_size - (void*) &sb);

  int err = stat("stat.c", &sb);
  if (err >= 0) {
    err = sb.st_size; 
  }
  return err;
}
