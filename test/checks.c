#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  putc('P', stderr);
  char *p = malloc(12);
  free(p);

  sleep(5);

  printf("sleep\n");
  putc('P', stderr);
  p = malloc(12);
  free(p);

  return EXIT_SUCCESS;
}
