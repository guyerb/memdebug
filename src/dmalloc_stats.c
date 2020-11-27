#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <sys/queue.h>

enum bucket_index {
  BUCKET_0000, BUCKET_0004, BUCKET_0008, BUCKET_0016, BUCKET_0032, BUCKET_0064,
  BUCKET_0128, BUCKET_0256, BUCKET_0512, BUCKET_1024, BUCKET_2048, BUCKET_4096,
};
#define BUCKETS_NDX_NUM 12	/* N.B. make sure to adjust if you increase enums above */
#define BUCKETS_AGE_NUM  1000

struct dmalloc_stats {
  uint32_t s_allocated_current;
  uint32_t s_allocated_alltime;
  uint32_t s_buckets_size[BUCKETS_NDX_NUM];
  uint32_t s_buckets_age[BUCKETS_AGE_NUM];
  uint32_t s_null_frees;
  uint32_t s_failed_allocs;
  uint32_t s_invalid_birthdays;
  time_t   s_buckets_age_lastupdate;

  /* internal debugging */
  uint32_t _s_failed_malloc_locks;
};

static struct dmalloc_stats stats = {0};

/*
 * we monitor the age of each allocation by grouping them in buckets,
 * where each bucket represents one second of age. We bump them
 * forward by elapsed time since last update. For a full discussion
 * see NOTES sections of top-level README.
 */
static void dmalloc_age_buckets_update()
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  time_t elapsed, now;

  pthread_mutex_lock(&mutex);
  now = time(NULL);
  elapsed = now - stats.s_buckets_age_lastupdate;
  stats.s_buckets_age_lastupdate = now;
  pthread_mutex_unlock(&mutex);

  if (elapsed > 0) {
    for (int i = BUCKETS_AGE_NUM; i > 0; i--) {
      int src_ndx = i - 1;
      int dst_ndx = src_ndx + elapsed;
      time_t src_val = stats.s_buckets_age[src_ndx];

      if (src_val) stats.s_buckets_age[src_ndx] = 0;

      if (src_ndx == 999) continue;	/* bucket 1000 doesn't age */

      if (dst_ndx >= 999)
	stats.s_buckets_age[999] += src_val;
      else
	stats.s_buckets_age[dst_ndx] = src_val; /* not +- cuz residents moved */
    }
  }
}

static void dmalloc_age_bucket_insert()
{
  stats.s_buckets_age[0]++;
}
/* This element was allocated sometime in the past as indicated by
   age. We can determine how old it is by subtracting the current time
   from that age then compute the bucket index. If we don't find any
   elements in that bucket we will proceed down the list and decrease
   the population by one at the next populated bucket */
static void dmalloc_age_bucket_delete(time_t birth)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  time_t age, now;

  now = time(NULL);

  if (birth > now) {
    stats.s_invalid_birthdays++;
    return;
  }

  age = time(NULL) - birth;

  if (age > 999) age = 999;

  for (int i = age; i <= 999; i++) {
    int death = 0;

    pthread_mutex_lock(&mutex);
    if (stats.s_buckets_age[i] != 0) {
      stats.s_buckets_age[i]--;
      death = 1;
    }
    pthread_mutex_lock(&mutex);
    if (death)  break;
  }
}

static int size_bucket_ndx(size_t sz)
{
  if (sz >= 0x1000)
    return BUCKET_4096;
  if (sz & 0x0800)
    return BUCKET_2048;
  if (sz & 0x0400)
    return BUCKET_1024;
  if (sz & 0x0200)
    return BUCKET_0512;
  if (sz & 0x0100)
    return BUCKET_0256;
  if (sz & 0x0080)
    return BUCKET_0128;
  if (sz & 0x0040)
    return BUCKET_0064;
  if (sz & 0x0020)
    return BUCKET_0032;
  if (sz & 0x0010)
    return BUCKET_0016;
  if (sz & 0x0008)
    return BUCKET_0008;
  if (sz & 0x0004)
    return BUCKET_0004;
  return BUCKET_0000;
}

void dmalloc_stats_newalloc(void *ptr, size_t sz)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if (pthread_mutex_lock(&mutex) == 0) {
    if (ptr) {
      int ndx = size_bucket_ndx(sz);

      stats.s_allocated_alltime += sz;
      stats.s_allocated_current += sz;
      stats.s_buckets_size[ndx]++;
    } else {
      stats.s_failed_allocs++;
    }
    pthread_mutex_unlock(&mutex);
  } else {
    stats._s_failed_malloc_locks++;
  }

  dmalloc_age_buckets_update();
  dmalloc_age_bucket_insert();

  return;
}

void dmalloc_stats_newfree(void *ptr, size_t sz, time_t birth)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  int bucket = size_bucket_ndx(sz);

  pthread_mutex_lock(&mutex);
  if (ptr) {
    stats.s_allocated_current -= sz;
    stats.s_buckets_size[bucket]--;
  } else {
    stats.s_failed_allocs++;
  }
  pthread_mutex_unlock(&mutex);

  dmalloc_age_bucket_delete(birth);

  return;
}

/* --------------------------------------------------------------------------- */

static int unit_total;
static int unit_passed;
static int unit_failed;

void dmalloc_stats_delim()
{
  printf("unit total tests %d passed %d failed %d\n", unit_total, unit_passed, unit_failed);

  unit_total = 0;
  unit_failed = 0;
  unit_passed = 0;
}

void dmalloc_stats_start( char *descr)
{
  puts("------------------------------------------------------------------------------");
  printf("%s:\n", descr);
}

void dmalloc_stats_check(char *descr, uint32_t expected, uint32_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10u actual %10u\n", descr, passed ? "PASSED" : "FAILED", expected, actual);
}

#ifdef DMALLOC_STATS_UNIT
int main()
{
  printf("dmalloc stat unit tests:\n\n");
  dmalloc_stats_start("exercise size_bucket_ndx()");
  dmalloc_stats_check("malloc of 0 bytes", 0, size_bucket_ndx(0));
  dmalloc_stats_check("malloc of 1 bytes", 0, size_bucket_ndx(1));
  dmalloc_stats_check("malloc of 2 bytes", 0, size_bucket_ndx(2));
  dmalloc_stats_check("malloc of 3 bytes", 0, size_bucket_ndx(3));
  dmalloc_stats_check("malloc of 4 bytes", 1, size_bucket_ndx(4));
  dmalloc_stats_check("malloc of 5 bytes", 1, size_bucket_ndx(5));
  dmalloc_stats_check("malloc of 6 bytes", 1, size_bucket_ndx(6));
  dmalloc_stats_check("malloc of 7 bytes", 1, size_bucket_ndx(7));
  dmalloc_stats_check("malloc of 8 bytes", 2, size_bucket_ndx(8));
  dmalloc_stats_check("malloc of 15 bytes", 2, size_bucket_ndx(15));
  dmalloc_stats_check("malloc of 2047 bytes", 9, size_bucket_ndx(2047));
  dmalloc_stats_check("malloc of 2049 bytes", 10, size_bucket_ndx(2049));
  dmalloc_stats_check("malloc of 4095 bytes", 10, size_bucket_ndx(4095));
  dmalloc_stats_check("malloc of 4096 bytes", 11, size_bucket_ndx(4096));
  dmalloc_stats_check("malloc of 4097 bytes", 11, size_bucket_ndx(4097));
  dmalloc_stats_check("malloc of 1234567 bytes", 11, size_bucket_ndx(1234567));
  dmalloc_stats_delim();

  dmalloc_stats_start("exercise failed malloc and basic lock");
  dmalloc_stats_check("s_failed_allocs", 0, stats.s_failed_allocs);
  dmalloc_stats_check("_s_failed_malloc_locks", 0, stats._s_failed_malloc_locks);
  dmalloc_stats_newalloc(NULL, 12);
  dmalloc_stats_check("s_failed_allocs", 1, stats.s_failed_allocs);
  dmalloc_stats_check("_s_failed_malloc_locks", 0, stats._s_failed_malloc_locks);
  dmalloc_stats_delim();



}
#endif	/* DMALLOC_STATS_UNIT */
