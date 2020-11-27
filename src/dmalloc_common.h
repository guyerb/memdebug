#ifndef DMALLOC_COMMON_H
#define DMALLOC_COMMON_H

#include <time.h>

/* when tracking the allocated bytes we track usable size which may be
   a bit bigger than the size desired by the user. If user bothered to
   check usable size they could safely use their allocation up to that
   size. We don't however track the bookeeping overhead associated
   with an allocation. That would be a good improvement but for now we
   just attempt to track all bytes allocated that a user could use
   (even if they don't know about some of them 
*/
#ifdef LINUX
#include <malloc.h>
#define dmalloc_usable_size malloc_usable_size
#endif

#ifdef DARWIN
#include <malloc/malloc.h>
#define dmalloc_usable_size malloc_size
#endif

extern void dmalloc_stats_alloc(void *ptr, size_t sz, time_t now);
extern void dmalloc_stats_free(void *ptr, size_t sz, time_t now);
extern void dmalloc_printf( const char* format, ... );

static inline size_t dmalloc_extrabytes_sz()
{
  return sizeof(size_t);
}

static inline void *dmalloc_extrabytes_setandhide(void *ptr, size_t val)
{
  size_t *p = ptr;
  if (p) {
    *p = val;
    p++;;
  }
  return p;
}

static inline size_t dmalloc_extrabytes_get(void *ptr)
{
  size_t extra = 0;

  if (ptr)
    extra = *((time_t *)ptr - 1);
  
  return extra;
}

static inline void * dmalloc_basepointer_get(void *ptr)
{
  return (time_t *)ptr - 1;
}

#endif	/* DMALLOC_COMMON_H */
