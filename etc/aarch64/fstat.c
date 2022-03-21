#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
  struct stat sb;

  printf("sizeof: %ld\n", sizeof(struct stat));
  printf("offsetof: %ld\n", (void*) &sb.st_size - (void*) &sb);

  int fd = open("stat.c", O_RDWR);
  if (fd < 0) {
    return errno;
  } 
  int err = fstat(fd, &sb);
  if (err >= 0) {
    err = sb.st_size; 
    printf("size: %d\n", err);
  }
  return err;
}
