/* e.g. from  http://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/ */
#define _POSIX_C_SOURCE 200112L	/* enable fileno() */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  char buffer[1000];
  unsigned long amount_read;
  int fd;

  char *p = malloc(12);
  free(p);
  
  printf("type some text:\n");
  fd = fileno(stdin);

  amount_read = read(fd, buffer, sizeof buffer);

  printf("you typed:\n");
  if (fwrite(buffer, sizeof(char), amount_read, stdout) != amount_read) {
    perror("error writing");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
