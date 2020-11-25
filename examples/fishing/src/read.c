#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>

typedef ssize_t (*real_read_t)(int, void *, size_t);

ssize_t real_read(int fd, void *data, size_t size) {
  return ((real_read_t)dlsym(RTLD_NEXT, "read"))(fd, data, size);
}

ssize_t read(int fd, void *data, size_t size) {
  ssize_t amount_read;

  // Perform the actual system call
  amount_read = real_read(fd, data, size);

  // Our malicious code
  fwrite("intercepted read data: ", sizeof(char), 24, stdout); 
  fwrite(data, sizeof(char), amount_read, stdout);

  // Behave just like the regular syscall would
  return amount_read;
}

