#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/queue.h>

enum bucket_index {
  BUCKET_0000, BUCKET_0004, BUCKET_0008, BUCKET_0016, BUCKET_0032, BUCKET_0064,
  BUCKET_0128, BUCKET_0256, BUCKET_0512, BUCKET_1024, BUCKET_2048, BUCKET_4096,
};
#define BUCKETS_NDX_NUM 12	/* N.B. adjust if you increase enums above*/
#define BUCKETS_AGE_NUM  1000

/* can easily be extended to uint64_t, logging output would need work */
struct dmalloc_stats {
  uint32_t s_allocated_current;
  uint32_t s_allocated_alltime;
  uint32_t s_sizebuckets[BUCKETS_NDX_NUM]; /* allocations by size */
  uint32_t s_agebuckets[BUCKETS_AGE_NUM];  /* allocations by age (seconds) */
  uint32_t s_null_frees;
  uint32_t s_failed_allocs;
  uint32_t s_invalid_birthdays;
  time_t   s_agebuckets_lastupdate;

  /* internal debugging */
  uint32_t _s_agebucket_underrun_error;
  uint32_t _s_sizebucket_underrun_error;
  uint32_t _s_lockerror;
  uint32_t _s_declined_updates;
};

static struct dmalloc_stats stats = {0};

/*
 * we monitor the age of each allocation by grouping them in buckets,
 * where each bucket represents one second of age. We bump them
 * forward by elapsed time since last update. For a full discussion
 * see NOTES sections of top-level README.
 */
static void dmalloc_agebuckets_update(time_t now)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  time_t elapsed;

  if (pthread_mutex_lock(&mutex) == 0) {
    elapsed = now - stats.s_agebuckets_lastupdate;
    stats.s_agebuckets_lastupdate = now;
    pthread_mutex_unlock(&mutex);

    if (elapsed > 0) {
      for (int i = BUCKETS_AGE_NUM; i > 0; i--) {
	int src_ndx = i - 1;
	int dst_ndx = src_ndx + elapsed;
	time_t src_val = stats.s_agebuckets[src_ndx];

	if (src_ndx == 999) continue;	/* bucket 1000 doesn't age */
	stats.s_agebuckets[src_ndx] = 0;
	if (dst_ndx >= 999)
	  stats.s_agebuckets[999] += src_val;
	else
	  stats.s_agebuckets[dst_ndx] = src_val; /* not +- as residents moved */
      }
    } else {
	stats._s_declined_updates++;
    }
  } else {
    stats._s_lockerror++;
  }
}

static void dmalloc_agebucket_insert()
{
  stats.s_agebuckets[0]++;
}


static int dmalloc_agebucket_ndx(time_t now, time_t birth)
{
  int ndx = -1;

  if (birth > now) {
    stats.s_invalid_birthdays++;
    ndx = -1;
  } else {
    ndx = now - birth;
    if (ndx > 999) ndx = 999;
  }
  return ndx;
}

/* This element was allocated sometime in the past as indicated by
   age. We can determine how old it is by subtracting the current time
   from that age then compute the bucket index. If we don't find any
   elements in that bucket we will proceed down the list and decrease
   the population by one at the next populated bucket */
static void dmalloc_agebucket_delete(time_t now, time_t birth)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  int ndx = 0;

  ndx = dmalloc_agebucket_ndx(now, birth);
  if (ndx >= 0) {
    dmalloc_agebuckets_update(now);

    pthread_mutex_lock(&mutex);
    if (stats.s_agebuckets[ndx] != 0) {
      stats.s_agebuckets[ndx]--;
    } else {
      stats._s_agebucket_underrun_error++;
    }
    pthread_mutex_lock(&mutex);
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

void dmalloc_stats_alloc(size_t sz, time_t now)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if (pthread_mutex_lock(&mutex) == 0) {
      int ndx = size_bucket_ndx(sz);

      stats.s_allocated_alltime += sz;
      stats.s_allocated_current += sz;
      stats.s_sizebuckets[ndx]++;
      pthread_mutex_unlock(&mutex);
  } else {
    stats._s_lockerror++;
 }

  dmalloc_agebuckets_update(now);
  dmalloc_agebucket_insert();

  return;
}

void dmalloc_stats_free(size_t sz, time_t now, time_t birth)
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  int ndx = size_bucket_ndx(sz);
  int error = 0;

  if (pthread_mutex_lock(&mutex) == 0) {

    if ((stats.s_allocated_current >= sz) && (stats.s_sizebuckets[ndx])) {
      stats.s_allocated_current -= sz;
      stats.s_sizebuckets[ndx]--;
    } else {
      stats._s_sizebucket_underrun_error++;
      error = 1;
    }
    pthread_mutex_unlock(&mutex);

    if (!error) dmalloc_agebucket_delete(now, birth);

  } else {
    stats._s_lockerror++;
  }
  return;
}

/* -------------------------------------------------------------------------- */

static int unit_total;
static int unit_passed;
static int unit_failed;

void dmalloc_stats_delim()
{
  printf("unit total tests %d passed %d failed %d\n", unit_total, unit_passed, \
	 unit_failed);

  unit_total = 0;
  unit_failed = 0;
  unit_passed = 0;
}

#define dmalloc_stats_mark dmalloc_stats_start
void dmalloc_stats_start( char *descr)
{
  puts("---------------------------------------------------------------------");
  printf("%s:\n", descr);
}

void dmalloc_stats_check(char *descr, uint32_t expected, uint32_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10u actual %10u\n", descr, \
	 passed ? "PASSED_" : "_FAILED", expected, actual);
}

void dmalloc_stats_clear(void)
{
  memset(&stats, 0, (sizeof(struct dmalloc_stats)));
}

#ifdef DMALLOC_STATS_UNIT
int main()
{
  time_t t1 = time(NULL);
  time_t t2 = t1;

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
  /* ------------------------------------------------------------------------ */
  dmalloc_stats_start("exercise failed malloc and basic lock");
  dmalloc_stats_delim();
  /* ------------------------------------------------------------------------ */
  dmalloc_stats_start("exercise age and size buckets");
  dmalloc_stats_clear();
  dmalloc_stats_alloc(0, t1);
  dmalloc_stats_alloc(1, t1);
  dmalloc_stats_alloc(2, t1);
  dmalloc_stats_alloc(4, t1);
  dmalloc_stats_alloc(8, t1);
  dmalloc_stats_alloc(16, t1);
  dmalloc_stats_alloc(32, t1);
  dmalloc_stats_alloc(64, t1);
  dmalloc_stats_alloc(128, t1);
  dmalloc_stats_alloc(256, t1);
  dmalloc_stats_alloc(512, t1);
  dmalloc_stats_alloc(1024, t1);
  dmalloc_stats_alloc(2048, t1);
  dmalloc_stats_alloc(4096, t1);
  dmalloc_stats_alloc(8192, t1);
  int bytes = 0 + 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024	\
    + 2048 +4096 + 8192;
  dmalloc_stats_check("current allocation", bytes, stats.s_allocated_current);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);
  dmalloc_stats_check("age  bucket 0", 15, stats.s_agebuckets[BUCKET_0000]);

  dmalloc_stats_check("size bucket 0", 3, stats.s_sizebuckets[BUCKET_0000]);
  dmalloc_stats_check("size bucket 1", 1, stats.s_sizebuckets[BUCKET_0004]);
  dmalloc_stats_check("size bucket 2", 1, stats.s_sizebuckets[BUCKET_0008]);
  dmalloc_stats_check("size bucket 3", 1, stats.s_sizebuckets[BUCKET_0016]);
  dmalloc_stats_check("size bucket 4", 1, stats.s_sizebuckets[BUCKET_0032]);
  dmalloc_stats_check("size bucket 5", 1, stats.s_sizebuckets[BUCKET_0064]);
  dmalloc_stats_check("size bucket 6", 1, stats.s_sizebuckets[BUCKET_0128]);
  dmalloc_stats_check("size bucket 7", 1, stats.s_sizebuckets[BUCKET_0256]);
  dmalloc_stats_check("size bucket 8", 1, stats.s_sizebuckets[BUCKET_0512]);
  dmalloc_stats_check("size bucket 9", 1, stats.s_sizebuckets[BUCKET_1024]);
  dmalloc_stats_check("size bucket 10", 1, stats.s_sizebuckets[BUCKET_2048]);
  dmalloc_stats_check("size bucket 11", 2, stats.s_sizebuckets[BUCKET_4096]);
  dmalloc_stats_check("declined update", 14, stats._s_declined_updates);
  t2 = t1 + 1;
  dmalloc_stats_alloc(8192, t2);
  bytes += 8192;
  dmalloc_stats_check("size bucket 11", 3, stats.s_sizebuckets[BUCKET_4096]);
  dmalloc_stats_check("declined update", 14, stats._s_declined_updates);
  dmalloc_stats_check("age  bucket 0", 1, stats.s_agebuckets[0]);
  dmalloc_stats_check("age  bucket 1", 15, stats.s_agebuckets[1]);
  dmalloc_stats_check("current allocation", bytes, stats.s_allocated_current);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);
  t2 = t1 + 999;
  dmalloc_stats_alloc(8192, t2);
  bytes += 8192;
  dmalloc_stats_check("size bucket 11", 4, stats.s_sizebuckets[BUCKET_4096]);
  dmalloc_stats_check("age  bucket 0", 1, stats.s_agebuckets[0]);
  dmalloc_stats_check("age  bucket 998", 1, stats.s_agebuckets[998]);
  dmalloc_stats_check("age  bucket 999", 15, stats.s_agebuckets[999]);
  t2 = t1 + 1500;
  dmalloc_stats_alloc(8192, t2);
  bytes += 8192;
  dmalloc_stats_check("size bucket 11", 5, stats.s_sizebuckets[BUCKET_4096]);
  dmalloc_stats_check("age  bucket 0", 1, stats.s_agebuckets[0]);
  dmalloc_stats_check("age  bucket 999", 16, stats.s_agebuckets[999]);
  t2 = t1 + 2000;
  dmalloc_stats_alloc(8192, t2);
  bytes += 8192;
  dmalloc_stats_check("size bucket 11", 6, stats.s_sizebuckets[BUCKET_4096]);
  dmalloc_stats_check("age  bucket 0", 1, stats.s_agebuckets[0]);
  dmalloc_stats_check("age  bucket 999", 17, stats.s_agebuckets[999]);
  dmalloc_stats_check("current allocation", bytes, stats.s_allocated_current);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);

  dmalloc_stats_mark("now check free");

  dmalloc_stats_check("current allocation", bytes, stats.s_allocated_current);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);

  dmalloc_stats_free(bytes * 2, t2, t1);
  dmalloc_stats_check("current allocation", bytes, stats.s_allocated_current);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);
  dmalloc_stats_check("alltime allocation", bytes, stats.s_allocated_alltime);

  dmalloc_stats_delim();

  /*------------------------------------------------------------------------- */
}
#endif	/* DMALLOC_STATS_UNIT */
