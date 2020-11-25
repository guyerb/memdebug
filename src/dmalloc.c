/* dmalloc: a simple malloc debugging library.
 *
 * See README and dmalloc.pdf for description of usage, output,
 * issues, etc. but basically:
 *
 *  - gather some stats on usage on every (c|m)alloc,realloc, free
 *  - print those stats to stderr a max of every five seconds
 */
#include <stdio.h>
#include <stdlib.h>

#include "dmalloc_common.h"
#include "libc_wrappers.h"

void * dmalloc_calloc_intercept(size_t count, size_t size)
{
  void *ptr;

  dmalloc_printf("dmalloc_calloc\n");
  /* alloc bytes and hide our birthday inside */
  ptr = libc_calloc_wrapper(count, size + dmalloc_extrabytes_sz());
  ptr = dmalloc_extrabytes_setandhide(ptr, time(NULL));

  dmalloc_stats_newalloc(ptr, dmalloc_usable_size(ptr));

  return ptr;
}

void dmalloc_free_intercept(void *ptr)
{
  dmalloc_printf("dmalloc_free\n");

  dmalloc_stats_newfree(ptr, dmalloc_usable_size(ptr), dmalloc_extrabytes_get(ptr));
  libc_free_wrapper(ptr);

  return;
}

void * dmalloc_malloc_intercept(size_t size)
{
  void *ptr;

  dmalloc_printf("dmalloc_malloc\n");

  /* alloc bytes and hide our birthday inside */
  ptr = libc_malloc_wrapper(size + dmalloc_extrabytes_sz());
  ptr = dmalloc_extrabytes_setandhide(ptr, time(NULL));

  dmalloc_stats_newalloc(ptr, dmalloc_usable_size(ptr));

  return ptr;
}

/* see TODO in README for thoughts on a better implementation */
void * dmalloc_realloc_intercept(void *ptr, size_t size)
{
  void *p;

  dmalloc_printf("dmalloc_realloc\n");

  dmalloc_stats_newfree(ptr, dmalloc_usable_size(ptr), dmalloc_extrabytes_get(ptr));

  /* alloc bytes and hide birthday inside */
  p = libc_realloc_wrapper(ptr, size + dmalloc_extrabytes_sz());
  p = dmalloc_extrabytes_setandhide(ptr, time(NULL));

  dmalloc_stats_newalloc(p, dmalloc_usable_size(p));

  return p;
}

#ifdef LINUX
void * calloc(size_t count, size_t size)
{
  return dmalloc_calloc_intercept(count, size);
}

void free(void *ptr)
{
  dmalloc_free_intercept(ptr);
}

void * malloc(size_t size)
{
  return dmalloc_malloc_intercept(size);
}

void * realloc(void *ptr, size_t size)
{
  return dmalloc_realloc_intercept(ptr, size);
}
#endif	/* LINUX */

#ifdef DARWIN
/* from dyld-interposing.h */
#define DYLD_INTERPOSE(_replacment,_replacee)				\
  __attribute__((used)) static struct{					\
    const void* replacment;						\
    const void* replacee;						\
  } _interpose_##_replacee __attribute__ ((section ("__DATA,__interpose"))) = { \
    (const void*)(unsigned long)&_replacment,				\
    (const void*)(unsigned long)&_replacee				\
  };

DYLD_INTERPOSE(dmalloc_calloc_intercept, calloc)
DYLD_INTERPOSE(dmalloc_free_intercept, free)
DYLD_INTERPOSE(dmalloc_malloc_intercept, malloc)
DYLD_INTERPOSE(dmalloc_realloc_intercept, realloc)

#endif	/* DARWIN */

