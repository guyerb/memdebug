#define _GNU_SOURCE		/* for dlsym */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "dmalloc_common.h"
#include "libc_wrappers.h"

typedef void * (*calloc_t)(size_t, size_t);
typedef void   (*free_t)(void *);
typedef void * (*malloc_t)(size_t);
typedef void * (*realloc_t)(void *, size_t);

static calloc_t	 libc_callocp = NULL;
static free_t	 libc_freep = NULL;
static malloc_t	 libc_mallocp = NULL;
static realloc_t libc_reallocp = NULL;

/* my_malloc() - simple memory for early allocations from dlym(),
   before we have resolved libc routines */
#define DMALLOC_PREINIT_ROWS 5
#define DMALLOC_PREINIT_SIZE 256
static uint8_t dmalloc_preinit_buffer[DMALLOC_PREINIT_ROWS][DMALLOC_PREINIT_SIZE];
static uint8_t dmalloc_row = 0;

#define UNUSED(x) (void)(x)

static void * my_malloc(size_t size)
{
  void *ptr;
  UNUSED(size);

  ptr = &dmalloc_preinit_buffer[dmalloc_row][0];
  dmalloc_row = (dmalloc_row + 1) % DMALLOC_PREINIT_ROWS;

  return ptr;
}

void __attribute__ ((constructor)) libc_wrapper_init(void)
{
  /* fish for pointers to actual malloc routines */
  putc('!', stderr);
  libc_callocp = (calloc_t)dlsym(RTLD_NEXT, "calloc");
  libc_freep = (free_t)dlsym(RTLD_NEXT, "free");
  libc_mallocp = (malloc_t)dlsym(RTLD_NEXT, "malloc");
  libc_reallocp = (realloc_t)dlsym(RTLD_NEXT, "realloc");
  dmalloc_printf("c %p, f %p, m %p r %p\n", libc_callocp, libc_freep, libc_mallocp, libc_reallocp);
}

void * libc_calloc_wrapper(size_t count, size_t size)
{
  return libc_callocp(count, size);
}

void libc_free_wrapper(void *ptr)
{
  if (ptr >= (void *)dmalloc_preinit_buffer &&
      ptr < (void *)(dmalloc_preinit_buffer + sizeof(dmalloc_preinit_buffer))) {
    putc('e', stderr);
    return;
  }
  putc('F', stderr);
  libc_freep(ptr);
}

void * libc_malloc_wrapper(size_t size)
{
  void *p;

  if (!libc_mallocp) {
    putc('E', stderr);
    p = my_malloc(size);
  } else {
    putc('M', stderr);
    p = libc_mallocp(size);
  }
  return p;
}

void * libc_realloc_wrapper(void *ptr, size_t size)
{
  void *p;

  putc('R', stderr);
  p = libc_reallocp(ptr, size);
  return p;
}
