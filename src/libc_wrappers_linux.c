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
typedef void * (*reallocf_t)(void *, size_t);

static calloc_t	  libc_callocp = NULL;
static free_t	  libc_freep = NULL;
static malloc_t	  libc_mallocp = NULL;
static realloc_t  libc_reallocp = NULL;
static reallocf_t libc_reallocfp = NULL;

/* my_malloc() - simple memory for early allocations from dlym(),
   before we have resolved libc routines */
#define DMALLOC_PREINIT_ROWS 10
#define DMALLOC_PREINIT_SIZE 2048
static uint8_t dmalloc_preinit_buffer[DMALLOC_PREINIT_ROWS][DMALLOC_PREINIT_SIZE];
static uint8_t dmalloc_row = 0;

#define UNUSED(x) (void)(x)

static void * my_malloc(size_t size)
{
  void *ptr;
  UNUSED(size);

  ptr = &dmalloc_preinit_buffer[dmalloc_row][0];
  memset(ptr, 0, DMALLOC_PREINIT_SIZE);
  dmalloc_row = (dmalloc_row + 1) % DMALLOC_PREINIT_ROWS;

  return ptr;
}

void __attribute__ ((constructor)) libc_wrapper_init(void)
{
  /* fish for pointers to actual malloc routines */
  dputc('!', stderr);
  libc_callocp = (calloc_t)dlsym(RTLD_NEXT, "calloc");
  libc_freep = (free_t)dlsym(RTLD_NEXT, "free");
  libc_mallocp = (malloc_t)dlsym(RTLD_NEXT, "malloc");
  libc_reallocfp = (realloc_t)dlsym(RTLD_NEXT, "reallocf");
  libc_reallocp = (realloc_t)dlsym(RTLD_NEXT, "realloc");
  //  dmalloc_printf("c %p, f %p, m %p r %p\n", libc_callocp, libc_freep, libc_mallocp, libc_reallocp);
  dputc('!', stderr);
}

int libc_wrappers_initialized()
{
  return (libc_reallocp != NULL);
}

void * libc_calloc_wrapper(size_t count, size_t size)
{
  void *ptr;

  dputc('C', stderr);
  if (!libc_callocp) {
    dputc('E', stderr);
    ptr = my_malloc(size);
  } else {
    dputc('>', stderr);
    ptr = libc_callocp(count, size);
  }
  return ptr;
}

void libc_free_wrapper(void *ptr)
{
  dputc('F', stderr);
  if (ptr >= (void *)dmalloc_preinit_buffer &&
      ptr < ((void *)(dmalloc_preinit_buffer + sizeof(dmalloc_preinit_buffer)))) {
    dputc('e', stderr);
  } else {
    if (libc_freep) {
      dputc('>', stderr);
      libc_freep(ptr);
    } else {
      dputc('?', stderr);	/* we can leak a bit on load (maybe a shell allocation? */
    }
  }
}

void * libc_malloc_wrapper(size_t size)
{
  void *ptr = NULL;

  dputc('M', stderr);
  if (!libc_mallocp) {
    dputc('E', stderr);
    ptr = my_malloc(size);
  } else {
    dputc('>', stderr);
    ptr = libc_mallocp(size);
  }
  return ptr;
}

void * libc_realloc_wrapper(void *ptr, size_t size)
{
  void *p = NULL;

  dputc('R', stderr);
  if (libc_reallocp) {
    dputc('>', stderr);
    p = libc_reallocp(ptr, size);
  } else {
    dputc('?', stderr);
  }
  return p;
}

void * libc_reallocf_wrapper(void *ptr, size_t size)
{
  void *p = NULL;

  dputc('S', stderr);
  if (libc_reallocp) {
    dputc('>', stderr);
    p = libc_reallocfp(ptr, size);
  } else {
    dputc('?', stderr);
  }
  return p;
}
