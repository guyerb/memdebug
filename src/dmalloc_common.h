#ifndef DMALLOC_COMMON_H
#define DMALLOC_COMMON_H

#include <time.h>
#include <stdint.h>

/* when tracking the allocated bytes we track usable size which may be
   a bigger than the size desired by the user.

   N.B. Not actually using these because the numbers are quite large
   and really skew the reporting and usefulness of the library
*/
#ifdef LINUX
#include <malloc.h>
#define dmalloc_usable_size malloc_usable_size
#endif	/* LINUX */

#ifdef DARWIN
#include <malloc/malloc.h>
#define dmalloc_usable_size malloc_size
#endif	/* DARWIN */

#ifdef DMALLOC_DEBUG
#define dputc putc
#else
#define dputc(x, y) do {} while(0)
#endif

/* | MAGIC | TSTAMP | SZ | USER DATA | */
#define DMAGIC 0xDACABEEF
#define DSIZE  (sizeof(time_t) + sizeof(uint32_t) + sizeof(size_t))
#define MSIZE  (sizeof(uint32_t))
#define TSIZE  (sizeof(time_t))
#define SSIZE  (sizeof(size_t))

extern void dmalloc_stats_alloc(size_t, time_t);
extern void dmalloc_stats_free(size_t, time_t, time_t);
extern void dmalloc_printf( const char* format, ... );
extern void dmalloc_logf( const char* format, ... );
extern void dmalloc_log_protect();
extern void dmalloc_log_unprotect();


extern void dmalloc_log_stats(); /* output the report. */

/* or use DSIZE */
static inline size_t dmalloc_cookies_sz()
{
  return DSIZE;
}

/* | MAGIC | TSTAMP | SZ | USER DATA | */
static inline void *dmalloc_cookies_setandhide(void *ptr, time_t birthday, size_t sz)
{
  void *p = NULL;

  if (ptr) {
    *(uint32_t *)ptr = DMAGIC;
    *(time_t *)((uint8_t *)ptr + MSIZE) = birthday;
    *(size_t *)((uint8_t *)ptr + MSIZE + TSIZE) = sz;
    p = (void *)((uint8_t *)ptr + DSIZE);
  }
  return p;
}

/* | MAGIC | TSTAMP | SZ | USER DATA | */
static inline size_t dmalloc_size_get(void *ptr)
{
  size_t sz = 0;
  uint32_t magic = 0;

  if (ptr) {
    uint32_t *pm = (uint32_t *)((uint8_t *)ptr - DSIZE);
    time_t   *ps = (time_t *)((uint8_t *)ptr - SSIZE);

    magic = *pm;
    sz = *ps;

    if (magic != DMAGIC) {
      sz = 0;
    }
  }
  return sz;
}

/* | MAGIC | TSTAMP | SZ | USER DATA | */
static inline time_t dmalloc_birthday_get(void *ptr)
{
  time_t birthday = 0;
  uint32_t magic = 0;

  if (ptr) {
    uint32_t *pm = (uint32_t *)((uint8_t *)ptr - DSIZE);
    time_t   *pt = (time_t *)((uint8_t *)ptr - SSIZE - TSIZE);

    magic = *pm;
    birthday = *pt;
    //    printf("%s %p pm %p pt %p magic %x b %0lx \n", __func__, ptr, pm, pt, magic, birthday);

    if (magic != DMAGIC) {
      birthday = 0;
    }
  }
  return birthday;
}

static inline int dmalloc_ptr_ours(void *ptr)
{
  if (dmalloc_birthday_get(ptr) != 0L)
    return 1;
  else
    return 0;
}

static inline void * dmalloc_basepointer_get(void *ptr)
{
  return (void *)((uint8_t *)(ptr - DSIZE));
}

#endif	/* DMALLOC_COMMON_H */
