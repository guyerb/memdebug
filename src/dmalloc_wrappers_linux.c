#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "dmalloc_wrappers.h"

typedef void * (*real_calloc_t)(size_t, size_t);
typedef void   (*real_free_t)(void *);
typedef void * (*real_malloc_t)(size_t);
typedef void * (*real_realloc_t)(void *, size_t);

static real_calloc_t real_callocp = NULL;
static real_free_t real_freep = NULL;
static real_malloc_t real_mallocp = NULL;
static real_realloc_t real_reallocp = NULL;

static uint8_t buffer[1024];
static int inited = 0;

#define UNUSED(x) (void)(x)

static void * my_calloc(size_t count, size_t size)
{
  UNUSED(count);
  UNUSED(size);
  
  memset(buffer, 0, sizeof(buffer));
  return buffer;
}

static void * my_realloc(void *ptr, size_t size)
{
  UNUSED(ptr);
  UNUSED(size);
  
  return buffer;
}

void __attribute__ ((constructor)) dmalloc_wrapper_init(void)
{

  puts("wrapper_init");
#if 0  
  real_callocp = (real_calloc_t)dlsym(RTLD_NEXT, "calloc");
  putchar('1');
  real_freep = (real_free_t)dlsym(RTLD_NEXT, "free");
  real_mallocp = (real_malloc_t)dlsym(RTLD_NEXT, "malloc");
  real_reallocp = (real_realloc_t)dlsym(RTLD_NEXT, "realloc");
  puts("exit");
  inited = 1;
  //  printf("calloc %p, free %p, malloc %p, realloc %p\n", (void *)real_callocp, (void *)real_freep, (void *)real_mallocp, (void *)real_reallocp);
#endif
}

void * dmalloc_calloc_wrapper(size_t count, size_t size)
{
  if (!real_callocp)
    return my_calloc(count, size);
  else
    return real_callocp(count, size);
}

void real_free(void *ptr)
{
  if (ptr == buffer) return;
  real_freep(ptr);
}

void * dmalloc_malloc_wrapper(size_t size)
{
  puts("M");
  return real_mallocp(size);
}

void * dmalloc_realloc_wrapper(void *ptr, size_t size)
{
  void *p;
  
  if (!inited)
    p = my_realloc(ptr, size);
  else
    p = real_reallocp(ptr, size);
  return p; 
}
