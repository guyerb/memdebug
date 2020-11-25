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

static uint8_t buffer[1024];
static uint8_t *pbuf = buffer + sizeof(size_t); /* leave room for birthday */

static int inited = 0;

#define UNUSED(x) (void)(x)

/* fishing for function pointers results in early calls to calloc, fake it */
static void * my_calloc(size_t count, size_t size)
{
  UNUSED(count);
  UNUSED(size);
  
  memset(buffer, 0, sizeof(buffer));
  return pbuf;
}

/* fishing for function pointers results in early calls to realloc(?), fake it */
static void * my_realloc(void *ptr, size_t size)
{
  UNUSED(ptr);
  UNUSED(size);
  
  return pbuf;
}

void __attribute__ ((constructor)) libc_wrapper_init(void)
{
  dmalloc_printf("libc_wrapper_init\n");

  /* fish for pointers to actual malloc routines */
  libc_callocp = (calloc_t)dlsym(RTLD_NEXT, "calloc");
  libc_freep = (free_t)dlsym(RTLD_NEXT, "free");
  libc_mallocp = (malloc_t)dlsym(RTLD_NEXT, "malloc");
  libc_reallocp = (realloc_t)dlsym(RTLD_NEXT, "realloc");
  inited = 1;
}

void * libc_calloc_wrapper(size_t count, size_t size)
{
  if (!libc_callocp)
    return my_calloc(count, size);
  else
    return libc_callocp(count, size);
}

void libc_free_wrapper(void *ptr)
{
  if (ptr == buffer) return;
  libc_freep(ptr);
}

void * libc_malloc_wrapper(size_t size)
{
  return libc_mallocp(size);
}

void * libc_realloc_wrapper(void *ptr, size_t size)
{
  void *p;
  
  if (!inited)
    p = my_realloc(ptr, size);
  else
    p = libc_reallocp(ptr, size);
  return p; 
}
