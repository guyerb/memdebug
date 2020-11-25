#include <string.h>

ssize_t read(int fd, void *data, size_t size) {
  strcpy(data, "I love cats\n");
  return 12;
}
