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
#include <stdint.h>

#include "dmalloc_common.h"
#include "libc_wrappers.h"

#ifndef DMALLOC_PASSTHROUGH
void * dmalloc_calloc_intercept(size_t count, size_t size)
{
  void *ptr = NULL;
  time_t now = time(NULL);

  dputc('c', stderr);
  ptr = libc_calloc_wrapper(count, size + DSIZE);
  if (ptr) {
    ptr = dmalloc_cookies_setandhide(ptr, now, size);
    dmalloc_stats_alloc(size, now);
  }
  return ptr;
}

void dmalloc_free_intercept(void *ptr)
{
  dputc('f', stderr);
  if (dmalloc_ptr_ours(ptr)) {
    dmalloc_stats_free(dmalloc_size_get(ptr), time(NULL), \
		       dmalloc_birthday_get(ptr));
    ptr = dmalloc_basepointer_get(ptr);
  }
  libc_free_wrapper(ptr);
  return;
}

void * dmalloc_malloc_intercept(size_t size)
{
  time_t now = time(NULL);
  void *ptr;

  dputc('m', stderr);
  ptr = libc_malloc_wrapper(size + DSIZE);
  if (ptr) {
    ptr = dmalloc_cookies_setandhide(ptr, now, size);
    dmalloc_stats_alloc(size, now);
  }
  return ptr;
}

void * dmalloc_realloc_intercept(void *ptr, size_t size)
{
  void *p;
  time_t now = time(NULL);

  dputc('r', stderr);
  if (ptr && dmalloc_ptr_ours(ptr)) {
    dmalloc_stats_free(dmalloc_size_get(ptr), time(NULL),	\
		       dmalloc_birthday_get(ptr));
    ptr = dmalloc_basepointer_get(ptr);
  }

  p = libc_realloc_wrapper(ptr, size + dmalloc_cookies_sz());
  if (p) {
    p = dmalloc_cookies_setandhide(p, now, size);
    dmalloc_stats_alloc(size, now);
  }
  return p;
}

void * dmalloc_reallocf_intercept(void *ptr, size_t size)
{
  return dmalloc_realloc_intercept(ptr, size);
}
#else  /* DMALLOC_PASSTHROUGH */

#warning "dmalloc library functionality disabled"

void * dmalloc_calloc_intercept(size_t count, size_t size)
{
  dputc('c', stderr);
  return libc_calloc_wrapper(count, size);
}

void dmalloc_free_intercept(void *ptr)
{
  dputc('f', stderr);
  libc_free_wrapper(ptr);
  return;
}

void * dmalloc_malloc_intercept(size_t size)
{
  dputc('m', stderr);
  return libc_malloc_wrapper(size);
}

void * dmalloc_realloc_intercept(void *ptr, size_t size)
{
  dputc('r', stderr);
  return libc_realloc_wrapper(ptr, size + dmalloc_cookies_sz());
}

void * dmalloc_reallocf_intercept(void *ptr, size_t size)
{
  dputc('s', stderr);
  return libc_reallocf_wrapper(ptr, size);
}
#endif	/* DMALLOC_PASSTHROUGH */


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

/* ------------------------------------------------------------------------- */
#ifdef DMALLOC_UNIT_TEST_WRAPPERS
/* ------------------------------------------------------------------------- */
#include <string.h>
#include <limits.h>

static int unit_total;
static int unit_passed;
static int unit_failed;

void ut_delim()
{
  printf("unit total tests %d passed %d failed %d\n", unit_total, unit_passed, \
	 unit_failed);

  unit_total = 0;
  unit_failed = 0;
  unit_passed = 0;
}

#define ut_mark ut_start
void ut_start( char *descr)
{
  puts("---------------------------------------------------------------------");
  printf("DMALLOC UNIT TESTS - %s:\n\n", descr);
}

void ut_check_sz(char *descr, size_t expected, size_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10lld actual %10lld\n", descr,	\
	 passed ? "PASSED_" : "_FAILED", (long long)expected, (long long)actual);

}

void ut_check_time(char *descr, time_t expected, time_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10llx actual %10llx\n", descr,	\
	 passed ? "PASSED_" : "_FAILED", (long long)expected, (long long)actual);
}

void ut_check_ptr(char *descr, void *expected, void *actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %p actual %p\n", descr,	\
	 passed ? "PASSED_" : "_FAILED", expected, actual);
}

void ut_check(char *descr, uint32_t expected, uint32_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10u actual %10u\n", descr, \
	 passed ? "PASSED_" : "_FAILED", expected, actual);
}

void ut_dump(uint8_t *ptr, size_t sz)
{
  printf("%s - %ld bytes\n", __func__, sz);
  for (size_t i = 0; i < sz; ) {
    printf("%3x", *ptr++);
    i++;
    if (i % 16 == 0 ) printf("\n");
  }
  printf("\n");
}

void ut_set(uint8_t *ptr, uint8_t ch, size_t sz)
{
  memset(ptr, ch, sz);
}

int main()
{
  uint8_t buf[64] = {0xFF};

  ut_start("exercise intercept support routines");
  ut_delim();

  ut_check("bad magic", 0, dmalloc_ptr_ours(&buf[50]));

  /* | MAGIC | TSTAMP | SZ | USER DATA | */
  uint32_t *pm = (uint32_t *)buf;
  time_t *pt = (time_t *)((uint8_t *)(buf + MSIZE));
  size_t *ps = (size_t *)((uint8_t *)(buf + MSIZE + TSIZE));
  time_t now = time(NULL);

  *pm = DMAGIC;
  *pt = now;
  *ps = 12;

  ut_check_sz("magic area size", DSIZE, dmalloc_cookies_sz());
  ut_check_sz("ptr sz", 12, dmalloc_size_get(buf + DSIZE));
  ut_check_sz("ptr birthday", now, dmalloc_birthday_get(buf + DSIZE));
  ut_check("good magic manual", 1, dmalloc_ptr_ours(buf + DSIZE));

  ut_set(buf, 0xff, 64);

  void *pb = buf;
  pb = dmalloc_cookies_setandhide(pb, now, 12);
  ut_check_ptr("set and hide", buf + DSIZE, pb);
  ut_check_ptr("check base ptr", buf, dmalloc_basepointer_get(pb));
  ut_check_time("check birthday", now, dmalloc_birthday_get(pb));
  ut_check_sz("check size", 12, dmalloc_size_get(pb));

  ut_start("exercise intercept routines");
  ut_delim();
  ut_check("libc constructors valid", 1, libc_wrappers_initialized());

  for (int i = 0; i < 5; i++) {
    void *p = malloc(12);
    free(p);
    p = calloc(1, 12);
    free(p);
  }
  dmalloc_log_stats();
}

#endif  /* DMALLOC_UNIT_TEST_WRAPPERS */
