#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
  int fd = open("read_file.obl", 2);
  char *buffer = (char*) mmap(0, 1024, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (buffer == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  memset(buffer, 0, 1024);
  read(fd, buffer, strlen(buffer));
  close(fd);
  write(1, buffer, 1024);
  return 0;
}

